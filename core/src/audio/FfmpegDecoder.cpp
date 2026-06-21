// Broad-format decode backend (MP3/AAC/OGG/ALAC/FLAC/AIFF/...) via FFmpeg/libav.
// Compiled only when DJ_WITH_AUDIO_BACKENDS is enabled; the dependency-free WAV
// backend (WavIo.cpp) is always available and is the default for .wav inputs.
//
// Output is normalized to the canonical internal representation: float32 planar
// PCM at the file's native sample rate, matching the WAV backend so analyzers
// and processors are backend-agnostic (plan §1).

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "djcore/audio/AudioDecoder.h"
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
  void operator()(AVFormatContext* c) const { if (c) avformat_close_input(&c); }
};
struct CodecCtxDeleter {
  void operator()(AVCodecContext* c) const { if (c) avcodec_free_context(&c); }
};
struct SwrDeleter {
  void operator()(SwrContext* s) const { if (s) swr_free(&s); }
};
struct PacketDeleter {
  void operator()(AVPacket* p) const { if (p) av_packet_free(&p); }
};
struct FrameDeleter {
  void operator()(AVFrame* f) const { if (f) av_frame_free(&f); }
};

class FfmpegDecoder : public AudioDecoder {
 public:
  explicit FfmpegDecoder(const std::string& path) { decode(path); }

  const FormatInfo& format() const override { return format_; }

  std::size_t readBlock(PcmBuffer& out, std::size_t maxFrames) override {
    const std::size_t remaining = pcm_.frames() - readPos_;
    const std::size_t n = std::min(maxFrames, remaining);
    out = PcmBuffer(pcm_.channels(), pcm_.sampleRate(), n);
    for (int ch = 0; ch < pcm_.channels(); ++ch) {
      const float* src = pcm_.channel(ch) + readPos_;
      std::copy(src, src + n, out.channel(ch));
    }
    readPos_ += n;
    return n;
  }

  PcmBuffer readAll() override { return pcm_; }

 private:
  void decode(const std::string& path);

  FormatInfo format_;
  PcmBuffer pcm_;
  std::size_t readPos_ = 0;
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
  if (!dec) {
    throw AudioError("FFmpeg: cannot allocate decoder context");
  }
  avcodec_parameters_to_context(dec.get(), stream->codecpar);
  if (avcodec_open2(dec.get(), codec, nullptr) < 0) {
    throw AudioError("FFmpeg: cannot open decoder");
  }

  const int channels = dec->ch_layout.nb_channels;
  const int sampleRate = dec->sample_rate;

  // Resample/convert everything to planar float at the native rate.
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
    std::vector<float*> outPtrs(channels);
    std::vector<std::vector<float>> tmp(channels, std::vector<float>(maxOut));
    for (int c = 0; c < channels; ++c) outPtrs[c] = tmp[c].data();
    const int n = swr_convert(swr.get(), reinterpret_cast<uint8_t**>(outPtrs.data()),
                              maxOut, f ? const_cast<const uint8_t**>(f->data) : nullptr,
                              f ? f->nb_samples : 0);
    for (int c = 0; c < channels && n > 0; ++c) {
      planes[c].insert(planes[c].end(), tmp[c].begin(), tmp[c].begin() + n);
    }
  };

  while (av_read_frame(fmt.get(), pkt.get()) >= 0) {
    if (pkt->stream_index == streamIdx) {
      if (avcodec_send_packet(dec.get(), pkt.get()) == 0) {
        while (avcodec_receive_frame(dec.get(), frame.get()) == 0) {
          drain(frame.get());
        }
      }
    }
    av_packet_unref(pkt.get());
  }
  avcodec_send_packet(dec.get(), nullptr);
  while (avcodec_receive_frame(dec.get(), frame.get()) == 0) drain(frame.get());
  drain(nullptr);  // flush resampler

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
  format_.durationMs = sampleRate ? static_cast<std::int64_t>(frames) * 1000 / sampleRate : 0;
  const AVCodecDescriptor* desc = avcodec_descriptor_get(dec->codec_id);
  format_.sourceIsLossy = desc && (desc->props & AV_CODEC_PROP_LOSSY) &&
                          !(desc->props & AV_CODEC_PROP_LOSSLESS);
}

}  // namespace

std::unique_ptr<AudioDecoder> openFfmpegDecoder(const std::string& path) {
  return std::make_unique<FfmpegDecoder>(path);
}

}  // namespace djcore
