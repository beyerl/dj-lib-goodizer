#pragma once

#include <cstdint>
#include <string>

namespace djcore {

// Container/codec metadata for a decoded (or to-be-encoded) audio file.
struct FormatInfo {
  std::string container;      // e.g. "wav", "flac", "mp3"
  std::string codec;          // e.g. "pcm_s16le", "pcm_f32le", "mp3"
  int sampleRate = 0;         // Hz
  int bitDepth = 0;           // bits/sample; 0 for compressed/unknown
  int channels = 0;
  std::int64_t durationMs = 0;
  bool sourceIsLossy = false;  // true for MP3/AAC/OGG etc.
};

}  // namespace djcore
