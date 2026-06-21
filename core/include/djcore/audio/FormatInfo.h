#pragma once

#include <cstdint>
#include <string>

namespace djcore {

// Container/codec metadata probed from a source file (FR-FMT-1).
struct FormatInfo {
  std::string container;        // e.g. "wav", "flac", "mp3", "aiff"
  std::string codec;            // e.g. "pcm_s16le", "flac", "mp3", "aac"
  int sampleRate = 0;           // Hz
  int bitDepth = 0;             // bits/sample; 0 if not applicable (compressed)
  int channels = 0;
  std::int64_t durationMs = 0;
  bool sourceIsLossy = false;   // FR-FMT-6: warn — standardizing won't restore quality
};

}  // namespace djcore
