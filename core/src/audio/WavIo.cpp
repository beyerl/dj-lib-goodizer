#include "djcore/audio/WavIo.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <vector>

#include "djcore/audio/AudioError.h"

namespace djcore {
namespace {

std::uint16_t readU16(const std::uint8_t* p) {
  return static_cast<std::uint16_t>(p[0] | (p[1] << 8));
}
std::uint32_t readU32(const std::uint8_t* p) {
  return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
         (static_cast<std::uint32_t>(p[2]) << 16) |
         (static_cast<std::uint32_t>(p[3]) << 24);
}

void putU16(std::vector<std::uint8_t>& v, std::uint16_t x) {
  v.push_back(static_cast<std::uint8_t>(x & 0xFF));
  v.push_back(static_cast<std::uint8_t>((x >> 8) & 0xFF));
}
void putU32(std::vector<std::uint8_t>& v, std::uint32_t x) {
  v.push_back(static_cast<std::uint8_t>(x & 0xFF));
  v.push_back(static_cast<std::uint8_t>((x >> 8) & 0xFF));
  v.push_back(static_cast<std::uint8_t>((x >> 16) & 0xFF));
  v.push_back(static_cast<std::uint8_t>((x >> 24) & 0xFF));
}
void putTag(std::vector<std::uint8_t>& v, const char* tag) {
  v.insert(v.end(), tag, tag + 4);
}

constexpr std::uint16_t kFormatPcm = 0x0001;
constexpr std::uint16_t kFormatFloat = 0x0003;
constexpr std::uint16_t kFormatExtensible = 0xFFFE;

}  // namespace

PcmBuffer readWav(const std::string& path, FormatInfo* infoOut) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw AudioError("cannot open WAV file: " + path);
  std::vector<std::uint8_t> buf((std::istreambuf_iterator<char>(in)),
                                std::istreambuf_iterator<char>());
  if (buf.size() < 44) throw AudioError("WAV file too small: " + path);

  if (std::memcmp(buf.data(), "RIFF", 4) != 0 ||
      std::memcmp(buf.data() + 8, "WAVE", 4) != 0) {
    throw AudioError("not a RIFF/WAVE file: " + path);
  }

  std::uint16_t formatTag = 0, channels = 0, bitsPerSample = 0;
  std::uint32_t sampleRate = 0;
  bool isFloat = false;
  bool haveFmt = false;
  const std::uint8_t* dataPtr = nullptr;
  std::size_t dataLen = 0;

  std::size_t pos = 12;  // after "RIFF"<size>"WAVE"
  while (pos + 8 <= buf.size()) {
    const std::uint8_t* chunk = buf.data() + pos;
    std::uint32_t chunkSize = readU32(chunk + 4);
    std::size_t bodyPos = pos + 8;
    if (bodyPos + chunkSize > buf.size()) {
      chunkSize = static_cast<std::uint32_t>(buf.size() - bodyPos);  // tolerate truncation
    }
    if (std::memcmp(chunk, "fmt ", 4) == 0 && chunkSize >= 16) {
      const std::uint8_t* f = buf.data() + bodyPos;
      formatTag = readU16(f);
      channels = readU16(f + 2);
      sampleRate = readU32(f + 4);
      bitsPerSample = readU16(f + 14);
      if (formatTag == kFormatExtensible && chunkSize >= 40) {
        std::uint16_t subFormat = readU16(f + 24);  // first 2 bytes of GUID
        isFloat = (subFormat == kFormatFloat);
      } else {
        isFloat = (formatTag == kFormatFloat);
      }
      haveFmt = true;
    } else if (std::memcmp(chunk, "data", 4) == 0) {
      dataPtr = buf.data() + bodyPos;
      dataLen = chunkSize;
    }
    pos = bodyPos + chunkSize + (chunkSize & 1u);  // chunks are word-aligned
  }

  if (!haveFmt) throw AudioError("WAV missing fmt chunk: " + path);
  if (!dataPtr) throw AudioError("WAV missing data chunk: " + path);
  if (channels == 0) throw AudioError("WAV has zero channels: " + path);

  const int bytesPerSample = bitsPerSample / 8;
  if (bytesPerSample == 0) throw AudioError("WAV unsupported bit depth: " + path);
  const std::size_t frameBytes = static_cast<std::size_t>(bytesPerSample) * channels;
  const std::size_t frames = frameBytes ? dataLen / frameBytes : 0;

  PcmBuffer out(channels, static_cast<int>(sampleRate), frames);

  for (std::size_t i = 0; i < frames; ++i) {
    for (int c = 0; c < channels; ++c) {
      const std::uint8_t* s = dataPtr + (i * frameBytes) + (c * bytesPerSample);
      float v = 0.0f;
      if (isFloat && bitsPerSample == 32) {
        std::uint32_t bits = readU32(s);
        std::memcpy(&v, &bits, sizeof(v));
      } else if (bitsPerSample == 16) {
        std::int16_t x = static_cast<std::int16_t>(readU16(s));
        v = static_cast<float>(x) / 32768.0f;
      } else if (bitsPerSample == 24) {
        std::int32_t x = s[0] | (s[1] << 8) | (s[2] << 16);
        if (x & 0x800000) x |= ~0xFFFFFF;  // sign-extend
        v = static_cast<float>(x) / 8388608.0f;
      } else if (bitsPerSample == 32) {
        std::int32_t x = static_cast<std::int32_t>(readU32(s));
        v = static_cast<float>(static_cast<double>(x) / 2147483648.0);
      } else if (bitsPerSample == 8) {
        v = (static_cast<float>(s[0]) - 128.0f) / 128.0f;  // 8-bit is unsigned
      } else {
        throw AudioError("WAV unsupported bit depth: " + path);
      }
      out.channel(c)[i] = v;
    }
  }

  if (infoOut) {
    infoOut->container = "wav";
    infoOut->codec = isFloat ? "pcm_f32le" : ("pcm_s" + std::to_string(bitsPerSample) + "le");
    infoOut->sampleRate = static_cast<int>(sampleRate);
    infoOut->bitDepth = bitsPerSample;
    infoOut->channels = channels;
    infoOut->durationMs =
        sampleRate ? static_cast<std::int64_t>(frames) * 1000 / sampleRate : 0;
    infoOut->sourceIsLossy = false;
  }
  return out;
}

FormatInfo probeWav(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw AudioError("cannot open WAV file: " + path);

  std::uint8_t riff[12];
  in.read(reinterpret_cast<char*>(riff), 12);
  if (in.gcount() < 12 || std::memcmp(riff, "RIFF", 4) != 0 ||
      std::memcmp(riff + 8, "WAVE", 4) != 0) {
    throw AudioError("not a RIFF/WAVE file: " + path);
  }

  std::uint16_t formatTag = 0, channels = 0, bitsPerSample = 0;
  std::uint32_t sampleRate = 0, dataLen = 0;
  bool isFloat = false, haveFmt = false, haveData = false;

  std::uint8_t hdr[8];
  while (in.read(reinterpret_cast<char*>(hdr), 8) && in.gcount() == 8) {
    std::uint32_t sz = readU32(hdr + 4);
    if (std::memcmp(hdr, "fmt ", 4) == 0) {
      std::vector<std::uint8_t> f(sz);
      in.read(reinterpret_cast<char*>(f.data()), sz);
      if (f.size() >= 16) {
        formatTag = readU16(f.data());
        channels = readU16(f.data() + 2);
        sampleRate = readU32(f.data() + 4);
        bitsPerSample = readU16(f.data() + 14);
        if (formatTag == kFormatExtensible && f.size() >= 26)
          isFloat = (readU16(f.data() + 24) == kFormatFloat);
        else
          isFloat = (formatTag == kFormatFloat);
        haveFmt = true;
      }
      if (sz & 1) in.seekg(1, std::ios::cur);
    } else if (std::memcmp(hdr, "data", 4) == 0) {
      dataLen = sz;
      haveData = true;
      in.seekg(static_cast<std::streamoff>(sz) + (sz & 1), std::ios::cur);
    } else {
      in.seekg(static_cast<std::streamoff>(sz) + (sz & 1), std::ios::cur);
    }
  }
  if (!haveFmt) throw AudioError("WAV missing fmt chunk: " + path);

  FormatInfo info;
  info.container = "wav";
  info.codec = isFloat ? "pcm_f32le" : ("pcm_s" + std::to_string(bitsPerSample) + "le");
  info.sampleRate = static_cast<int>(sampleRate);
  info.bitDepth = bitsPerSample;
  info.channels = channels;
  const int bytesPerSample = bitsPerSample / 8;
  const std::size_t frameBytes = static_cast<std::size_t>(bytesPerSample) * channels;
  const std::size_t frames = (haveData && frameBytes) ? dataLen / frameBytes : 0;
  info.durationMs =
      sampleRate ? static_cast<std::int64_t>(frames) * 1000 / sampleRate : 0;
  info.sourceIsLossy = false;
  return info;
}

void writeWav(const std::string& path, const PcmBuffer& buffer, int bitDepth,
              bool floatFormat) {
  const int channels = buffer.channels();
  const std::size_t frames = buffer.frames();
  if (channels <= 0) throw AudioError("writeWav: buffer has no channels");

  if (floatFormat) bitDepth = 32;
  if (!(bitDepth == 16 || bitDepth == 24 || bitDepth == 32)) {
    throw AudioError("writeWav: unsupported bit depth " + std::to_string(bitDepth));
  }
  const int bytesPerSample = bitDepth / 8;
  const std::uint32_t blockAlign = static_cast<std::uint32_t>(bytesPerSample) * channels;
  const std::uint32_t dataBytes = static_cast<std::uint32_t>(frames) * blockAlign;

  std::vector<std::uint8_t> hdr;
  hdr.reserve(44);
  putTag(hdr, "RIFF");
  putU32(hdr, 36 + dataBytes);
  putTag(hdr, "WAVE");
  putTag(hdr, "fmt ");
  putU32(hdr, 16);
  putU16(hdr, floatFormat ? kFormatFloat : kFormatPcm);
  putU16(hdr, static_cast<std::uint16_t>(channels));
  putU32(hdr, static_cast<std::uint32_t>(buffer.sampleRate()));
  putU32(hdr, static_cast<std::uint32_t>(buffer.sampleRate()) * blockAlign);  // byte rate
  putU16(hdr, static_cast<std::uint16_t>(blockAlign));
  putU16(hdr, static_cast<std::uint16_t>(bitDepth));
  putTag(hdr, "data");
  putU32(hdr, dataBytes);

  std::ofstream out(path, std::ios::binary);
  if (!out) throw AudioError("cannot create WAV file: " + path);
  out.write(reinterpret_cast<const char*>(hdr.data()),
            static_cast<std::streamsize>(hdr.size()));

  std::vector<std::uint8_t> row(blockAlign);
  for (std::size_t i = 0; i < frames; ++i) {
    for (int c = 0; c < channels; ++c) {
      const float f = buffer.channel(c)[i];
      std::uint8_t* d = row.data() + c * bytesPerSample;
      if (floatFormat) {
        std::uint32_t bits;
        std::memcpy(&bits, &f, sizeof(bits));
        d[0] = bits & 0xFF; d[1] = (bits >> 8) & 0xFF;
        d[2] = (bits >> 16) & 0xFF; d[3] = (bits >> 24) & 0xFF;
      } else if (bitDepth == 16) {
        long v = std::lround(f * 32768.0f);
        if (v > 32767) v = 32767;
        if (v < -32768) v = -32768;
        std::int16_t s = static_cast<std::int16_t>(v);
        d[0] = s & 0xFF; d[1] = (s >> 8) & 0xFF;
      } else if (bitDepth == 24) {
        long v = std::lround(static_cast<double>(f) * 8388608.0);
        if (v > 8388607) v = 8388607;
        if (v < -8388608) v = -8388608;
        std::int32_t s = static_cast<std::int32_t>(v);
        d[0] = s & 0xFF; d[1] = (s >> 8) & 0xFF; d[2] = (s >> 16) & 0xFF;
      } else {  // 32-bit int
        double dv = static_cast<double>(f) * 2147483648.0;
        if (dv > 2147483647.0) dv = 2147483647.0;
        if (dv < -2147483648.0) dv = -2147483648.0;
        std::int32_t s = static_cast<std::int32_t>(std::llround(dv));
        d[0] = s & 0xFF; d[1] = (s >> 8) & 0xFF;
        d[2] = (s >> 16) & 0xFF; d[3] = (s >> 24) & 0xFF;
      }
    }
    out.write(reinterpret_cast<const char*>(row.data()),
              static_cast<std::streamsize>(row.size()));
  }
  if (!out) throw AudioError("error writing WAV file: " + path);
}

}  // namespace djcore
