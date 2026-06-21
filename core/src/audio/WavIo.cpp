// Dependency-free WAV (RIFF/PCM + IEEE-float) decode/encode. This is the default
// audio backend and covers the reference test corpus and the WAV DJ format.
// Broad-format support (MP3/AAC/OGG/ALAC/FLAC/AIFF) is provided by the FFmpeg
// backend (FfmpegDecoder) when DJ_WITH_AUDIO_BACKENDS is enabled.

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "djcore/audio/AudioDecoder.h"
#include "djcore/audio/AudioEncoder.h"
#include "djcore/audio/AudioError.h"
#include "djcore/audio/Dither.h"
#include "djcore/audio/FormatInfo.h"
#include "djcore/audio/PcmBuffer.h"

namespace djcore {
namespace {

// ---- little-endian read/write helpers ---------------------------------------

std::uint32_t rdU32(const unsigned char* p) {
  return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
         (static_cast<std::uint32_t>(p[2]) << 16) |
         (static_cast<std::uint32_t>(p[3]) << 24);
}
std::uint16_t rdU16(const unsigned char* p) {
  return static_cast<std::uint16_t>(p[0]) | (static_cast<std::uint16_t>(p[1]) << 8);
}

void wrU32(std::ostream& os, std::uint32_t v) {
  unsigned char b[4] = {static_cast<unsigned char>(v), static_cast<unsigned char>(v >> 8),
                        static_cast<unsigned char>(v >> 16), static_cast<unsigned char>(v >> 24)};
  os.write(reinterpret_cast<char*>(b), 4);
}
void wrU16(std::ostream& os, std::uint16_t v) {
  unsigned char b[2] = {static_cast<unsigned char>(v), static_cast<unsigned char>(v >> 8)};
  os.write(reinterpret_cast<char*>(b), 2);
}
void wrTag(std::ostream& os, const char* tag) { os.write(tag, 4); }

constexpr std::uint16_t kFmtPcm = 0x0001;
constexpr std::uint16_t kFmtFloat = 0x0003;
constexpr std::uint16_t kFmtExtensible = 0xFFFE;

// ---- WAV decoder ------------------------------------------------------------

class WavDecoder : public AudioDecoder {
 public:
  explicit WavDecoder(const std::string& path) { load(path); }

  const FormatInfo& format() const override { return format_; }

  std::size_t readBlock(PcmBuffer& out, std::size_t maxFrames) override {
    const std::size_t remaining = pcm_.frames() - readPos_;
    const std::size_t n = std::min(maxFrames, remaining);
    out = PcmBuffer(pcm_.channels(), pcm_.sampleRate(), n);
    for (int ch = 0; ch < pcm_.channels(); ++ch) {
      const float* src = pcm_.channel(ch) + readPos_;
      std::memcpy(out.channel(ch), src, n * sizeof(float));
    }
    readPos_ += n;
    return n;
  }

  PcmBuffer readAll() override { return pcm_; }

 private:
  void load(const std::string& path);

  FormatInfo format_;
  PcmBuffer pcm_;
  std::size_t readPos_ = 0;
};

void WavDecoder::load(const std::string& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    throw AudioError("cannot open WAV file: " + path);
  }
  std::vector<unsigned char> buf((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());
  if (buf.size() < 12 || std::memcmp(buf.data(), "RIFF", 4) != 0 ||
      std::memcmp(buf.data() + 8, "WAVE", 4) != 0) {
    throw AudioError("not a RIFF/WAVE file: " + path);
  }

  std::uint16_t audioFormat = 0, channels = 0, bitsPerSample = 0;
  std::uint32_t sampleRate = 0;
  const unsigned char* data = nullptr;
  std::size_t dataSize = 0;

  std::size_t pos = 12;
  while (pos + 8 <= buf.size()) {
    const unsigned char* ck = buf.data() + pos;
    const std::uint32_t ckSize = rdU32(ck + 4);
    const std::size_t body = pos + 8;
    if (std::memcmp(ck, "fmt ", 4) == 0 && body + 16 <= buf.size()) {
      const unsigned char* p = buf.data() + body;
      audioFormat = rdU16(p);
      channels = rdU16(p + 2);
      sampleRate = rdU32(p + 4);
      bitsPerSample = rdU16(p + 14);
      if (audioFormat == kFmtExtensible && ckSize >= 40 && body + 26 <= buf.size()) {
        audioFormat = rdU16(p + 24);  // first 2 bytes of the subformat GUID
      }
    } else if (std::memcmp(ck, "data", 4) == 0) {
      data = buf.data() + body;
      dataSize = std::min<std::size_t>(ckSize, buf.size() - body);
    }
    pos = body + ckSize + (ckSize & 1u);  // chunks are word-aligned
  }

  if (channels == 0 || sampleRate == 0 || bitsPerSample == 0 || data == nullptr) {
    throw AudioError("malformed WAV (missing fmt/data): " + path);
  }

  const int bytesPerSample = bitsPerSample / 8;
  const std::size_t frameBytes = static_cast<std::size_t>(bytesPerSample) * channels;
  const std::size_t frames = frameBytes ? dataSize / frameBytes : 0;

  pcm_ = PcmBuffer(channels, static_cast<int>(sampleRate), frames);
  const bool isFloat = (audioFormat == kFmtFloat);

  for (std::size_t i = 0; i < frames; ++i) {
    for (int ch = 0; ch < channels; ++ch) {
      const unsigned char* s = data + (i * channels + ch) * bytesPerSample;
      float v = 0.0f;
      if (isFloat && bitsPerSample == 32) {
        std::uint32_t bits = rdU32(s);
        std::memcpy(&v, &bits, sizeof(float));
      } else if (bitsPerSample == 16) {
        v = static_cast<std::int16_t>(rdU16(s)) / 32768.0f;
      } else if (bitsPerSample == 24) {
        std::int32_t x = s[0] | (s[1] << 8) | (s[2] << 16);
        if (x & 0x800000) x |= ~0xFFFFFF;
        v = static_cast<float>(x) / 8388608.0f;
      } else if (bitsPerSample == 32) {  // 32-bit PCM int
        v = static_cast<float>(static_cast<std::int32_t>(rdU32(s))) / 2147483648.0f;
      } else if (bitsPerSample == 8) {  // unsigned 8-bit
        v = (static_cast<int>(s[0]) - 128) / 128.0f;
      } else {
        throw AudioError("unsupported WAV bit depth");
      }
      pcm_.channel(ch)[i] = v;
    }
  }

  format_.container = "wav";
  format_.codec = isFloat ? "pcm_f32le" : ("pcm_s" + std::to_string(bitsPerSample) + "le");
  format_.sampleRate = static_cast<int>(sampleRate);
  format_.bitDepth = bitsPerSample;
  format_.channels = channels;
  format_.durationMs = sampleRate ? static_cast<std::int64_t>(frames) * 1000 / sampleRate : 0;
  format_.sourceIsLossy = false;
}

std::string lowerExt(const std::string& path) {
  const auto dot = path.find_last_of('.');
  if (dot == std::string::npos) return {};
  std::string ext = path.substr(dot + 1);
  for (char& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return ext;
}

}  // namespace

#if DJ_WITH_AUDIO_BACKENDS
// Implemented in FfmpegDecoder.cpp when broad-format backends are enabled.
std::unique_ptr<AudioDecoder> openFfmpegDecoder(const std::string& path);
#endif

std::unique_ptr<AudioDecoder> openDecoder(const std::string& path) {
  const std::string ext = lowerExt(path);
  if (ext == "wav" || ext == "wave") {
    return std::make_unique<WavDecoder>(path);
  }
#if DJ_WITH_AUDIO_BACKENDS
  return openFfmpegDecoder(path);
#else
  throw AudioError("unsupported container '." + ext +
                   "'; rebuild with -DDJ_WITH_AUDIO_BACKENDS=ON for broad-format support");
#endif
}

void encodeToFile(const std::string& path, const PcmBuffer& pcm, const EncodeSpec& spec) {
  if (spec.container != "wav") {
#if DJ_WITH_AUDIO_BACKENDS
    // FFmpeg-based encoding for non-WAV containers is added with the backend.
    throw AudioError("non-WAV encoding requires the FFmpeg encoder backend");
#else
    throw AudioError("unsupported output container '" + spec.container +
                     "'; only WAV is available without DJ_WITH_AUDIO_BACKENDS");
#endif
  }

  std::ofstream os(path, std::ios::binary | std::ios::trunc);
  if (!os) {
    throw AudioError("cannot write WAV file: " + path);
  }

  const int channels = pcm.channels();
  const int bits = spec.isFloat ? 32 : spec.bitDepth;
  const int bytesPerSample = bits / 8;
  const std::uint32_t blockAlign = static_cast<std::uint32_t>(bytesPerSample) * channels;
  const std::uint32_t byteRate = static_cast<std::uint32_t>(spec.sampleRate) * blockAlign;
  const std::uint32_t dataSize =
      static_cast<std::uint32_t>(pcm.frames()) * blockAlign;

  wrTag(os, "RIFF");
  wrU32(os, 36 + dataSize);
  wrTag(os, "WAVE");
  wrTag(os, "fmt ");
  wrU32(os, 16);
  wrU16(os, spec.isFloat ? kFmtFloat : kFmtPcm);
  wrU16(os, static_cast<std::uint16_t>(channels));
  wrU32(os, static_cast<std::uint32_t>(spec.sampleRate));
  wrU32(os, byteRate);
  wrU16(os, static_cast<std::uint16_t>(blockAlign));
  wrU16(os, static_cast<std::uint16_t>(bits));
  wrTag(os, "data");
  wrU32(os, dataSize);

  TpdfDither dither;
  for (std::size_t i = 0; i < pcm.frames(); ++i) {
    for (int ch = 0; ch < channels; ++ch) {
      const float v = pcm.channel(ch)[i];
      if (spec.isFloat) {
        std::uint32_t bitsOut;
        std::memcpy(&bitsOut, &v, sizeof(float));
        wrU32(os, bitsOut);
      } else {
        const std::int32_t code = dither.quantize(v, bits);
        if (bits == 16) {
          wrU16(os, static_cast<std::uint16_t>(static_cast<std::int16_t>(code)));
        } else if (bits == 24) {
          const std::uint32_t u = static_cast<std::uint32_t>(code);
          unsigned char b[3] = {static_cast<unsigned char>(u),
                                static_cast<unsigned char>(u >> 8),
                                static_cast<unsigned char>(u >> 16)};
          os.write(reinterpret_cast<char*>(b), 3);
        } else if (bits == 32) {
          wrU32(os, static_cast<std::uint32_t>(code));
        } else {
          throw AudioError("unsupported output bit depth");
        }
      }
    }
  }
  if (!os) {
    throw AudioError("error while writing WAV file: " + path);
  }
}

}  // namespace djcore
