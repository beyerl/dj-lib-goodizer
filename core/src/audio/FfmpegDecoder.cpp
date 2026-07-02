// Broad-format decode backend (MP3/AAC/OGG/ALAC/FLAC/AIFF/...) via FFmpeg/libav.
// Compiled only when DJ_WITH_AUDIO_BACKENDS is enabled; the dependency-free WAV
// backend (WavIo.cpp) is always available and is the default for .wav inputs.
//
// Output is normalized to the canonical internal representation: float32 planar
// PCM at the file's native sample rate, matching the WAV backend so analyzers
// and processors are backend-agnostic.

#include "FfmpegBackend.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <vector>

#include "djcore/audio/AudioError.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

namespace djcore {
namespace {

struct FmtCtxDeleter {
  void operator()(AVFormatContext* c) const {
    if (c) avformat_close_input(&c);
  }
};
struct CodecCtxDeleter {
  void operator()(AVCodecContext* c) const {
    if (c) avcodec_free_context(&c);
  }
};
struct SwrDeleter {
  void operator()(SwrContext* s) const {
    if (s) swr_free(&s);
  }
};
struct PacketDeleter {
  void operator()(AVPacket* p) const {
    if (p) av_packet_free(&p);
  }
};
struct FrameDeleter {
  void operator()(AVFrame* f) const {
    if (f) av_frame_free(&f);
  }
};

std::string lowerExt(const std::string& path) {
  const auto slash = path.find_last_of("/\\");
  const auto dot = path.find_last_of('.');
  if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
    return "";
  }
  std::string ext = path.substr(dot + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext;
}

class FfmpegDecoder : public AudioDecoder {
 public:
  explicit FfmpegDecoder(const std::string& path) { decode(path); }
  const FormatInfo& format() const override { return format_; }
  PcmBuffer readAll() override { return pcm_; }

 private:
  void decode(const std::string& path);
  FormatInfo format_;
  PcmBuffer pcm_;
};

void FfmpegDecoder::decode(const std::string& path) {
  AVFormatContext* rawFmt = nullptr;
  if (avformat_open_input(&rawFmt, path.c_str(), nullptr, nullptr) < 0) {
    throw AudioError("FFmpeg: cannot open " + path);
  }
  std::unique_ptr<AVFormatContext, FmtCtxDeleter> fmt(rawFmt);

  if (avformat_find_stream_info(fmt.get(), nullptr) < 0) {
    throw AudioError("FFmpeg: cannot read stream info for " + path);
  }

  const AVCodec* codec = nullptr;
  const int streamIdx =
      av_find_best_stream(fmt.get(), AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
  if (streamIdx < 0 || !codec) {
    throw AudioError("FFmpeg: no audio stream in " + path);
  }
  AVStream* stream = fmt->streams[streamIdx];

  std::unique_ptr<AVCodecContext, CodecCtxDeleter> dec(avcodec_alloc_context3(codec));
  if (!dec) throw AudioError("FFmpeg: cannot allocate decoder context");
  avcodec_parameters_to_context(dec.get(), stream->codecpar);
  if (avcodec_open2(dec.get(), codec, nullptr) < 0) {
    throw AudioError("FFmpeg: cannot open decoder");
  }

  const int channels = dec->ch_layout.nb_channels;
  const int sampleRate = dec->sample_rate;
  if (channels <= 0 || sampleRate <= 0) {
    throw AudioError("FFmpeg: invalid channel/rate for " + path);
  }

  AVChannelLayout outLayout;
  av_channel_layout_default(&outLayout, channels);
  SwrContext* rawSwr = nullptr;
  swr_alloc_set_opts2(&rawSwr, &outLayout, AV_SAMPLE_FMT_FLTP, sampleRate,
                      &dec->ch_layout, dec->sample_fmt, sampleRate, 0, nullptr);
  std::unique_ptr<SwrContext, SwrDeleter> swr(rawSwr);
  if (!swr || swr_init(swr.get()) < 0) {
    throw AudioError("FFmpeg: cannot init resampler");
  }

  std::vector<std::vector<float>> planes(channels);
  std::unique_ptr<AVPacket, PacketDeleter> pkt(av_packet_alloc());
  std::unique_ptr<AVFrame, FrameDeleter> frame(av_frame_alloc());

  auto drain = [&](AVFrame* f) {
    const int maxOut = swr_get_out_samples(swr.get(), f ? f->nb_samples : 0);
    if (maxOut <= 0) return;
    std::vector<std::vector<float>> tmp(channels, std::vector<float>(maxOut));
    std::vector<float*> outPtrs(channels);
    for (int c = 0; c < channels; ++c) outPtrs[c] = tmp[c].data();
    const int n =
        swr_convert(swr.get(), reinterpret_cast<uint8_t**>(outPtrs.data()), maxOut,
                    f ? const_cast<const uint8_t**>(f->data) : nullptr,
                    f ? f->nb_samples : 0);
    for (int c = 0; c < channels && n > 0; ++c) {
      planes[c].insert(planes[c].end(), tmp[c].begin(), tmp[c].begin() + n);
    }
  };

  while (av_read_frame(fmt.get(), pkt.get()) >= 0) {
    if (pkt->stream_index == streamIdx) {
      if (avcodec_send_packet(dec.get(), pkt.get()) == 0) {
        while (avcodec_receive_frame(dec.get(), frame.get()) == 0) drain(frame.get());
      }
    }
    av_packet_unref(pkt.get());
  }
  avcodec_send_packet(dec.get(), nullptr);
  while (avcodec_receive_frame(dec.get(), frame.get()) == 0) drain(frame.get());
  drain(nullptr);  // flush the resampler

  const std::size_t frames = channels ? planes[0].size() : 0;
  pcm_ = PcmBuffer(channels, sampleRate, frames);
  for (int c = 0; c < channels; ++c) {
    std::copy(planes[c].begin(), planes[c].end(), pcm_.channel(c));
  }

  format_.container = fmt->iformat && fmt->iformat->name ? fmt->iformat->name : "";
  format_.codec = codec->name ? codec->name : "";
  format_.sampleRate = sampleRate;
  format_.channels = channels;
  format_.bitDepth = av_get_bytes_per_sample(dec->sample_fmt) * 8;
  format_.durationMs =
      sampleRate ? static_cast<std::int64_t>(frames) * 1000 / sampleRate : 0;
  const AVCodecDescriptor* desc = avcodec_descriptor_get(dec->codec_id);
  format_.sourceIsLossy = desc && (desc->props & AV_CODEC_PROP_LOSSY) &&
                          !(desc->props & AV_CODEC_PROP_LOSSLESS);
}

}  // namespace

std::unique_ptr<AudioDecoder> openFfmpegDecoder(const std::string& path) {
  return std::make_unique<FfmpegDecoder>(path);
}

FormatInfo probeFfmpegFormat(const std::string& path) {
  AVFormatContext* rawFmt = nullptr;
  if (avformat_open_input(&rawFmt, path.c_str(), nullptr, nullptr) < 0) {
    throw AudioError("FFmpeg: cannot open " + path);
  }
  std::unique_ptr<AVFormatContext, FmtCtxDeleter> fmt(rawFmt);
  if (avformat_find_stream_info(fmt.get(), nullptr) < 0) {
    throw AudioError("FFmpeg: cannot read stream info for " + path);
  }
  const AVCodec* codec = nullptr;
  const int streamIdx =
      av_find_best_stream(fmt.get(), AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);
  if (streamIdx < 0 || !codec) throw AudioError("FFmpeg: no audio stream in " + path);
  AVCodecParameters* par = fmt->streams[streamIdx]->codecpar;

  FormatInfo info;
  info.container = fmt->iformat && fmt->iformat->name ? fmt->iformat->name : "";
  info.codec = codec->name ? codec->name : "";
  info.sampleRate = par->sample_rate;
  info.channels = par->ch_layout.nb_channels;
  info.bitDepth = par->bits_per_raw_sample;  // 0 for many lossy codecs
  info.durationMs = fmt->duration > 0 ? fmt->duration * 1000 / AV_TIME_BASE : 0;
  const AVCodecDescriptor* desc = avcodec_descriptor_get(par->codec_id);
  info.sourceIsLossy = desc && (desc->props & AV_CODEC_PROP_LOSSY) &&
                       !(desc->props & AV_CODEC_PROP_LOSSLESS);
  return info;
}

bool ffmpegCanDecode(const std::string& path) {
  static const char* kExts[] = {"mp3", "flac", "ogg",  "oga", "opus", "aac",
                                "m4a", "mp4",  "alac", "aif", "aiff", "wma",
                                "wv",  "ape",  "mpc",  "ac3"};
  const std::string ext = lowerExt(path);
  for (const char* e : kExts)
    if (ext == e) return true;
  return false;
}

}  // namespace djcore
