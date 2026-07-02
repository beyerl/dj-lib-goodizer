#pragma once

#include <string>

#include "djcore/audio/PcmBuffer.h"

namespace djcore {

// How to encode an output file.
struct EncodeSpec {
  std::string container = "wav";  // "wav" now; more formats via FFmpeg later
  int bitDepth = 16;              // integer PCM bit depth
  bool floatFormat = false;       // if true, write 32-bit IEEE float
  bool dither = false;            // apply TPDF dither (on bit-depth reduction)
};

// Encodes `buffer` to `path` per `spec`. Throws AudioError on unsupported
// container or I/O failure.
void encodeToFile(const std::string& path, const PcmBuffer& buffer,
                  const EncodeSpec& spec);

}  // namespace djcore
