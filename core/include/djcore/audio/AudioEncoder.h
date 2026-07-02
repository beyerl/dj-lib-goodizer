#pragma once

#include <string>

#include "djcore/audio/PcmBuffer.h"

namespace djcore {

// How to encode an output file.
struct EncodeSpec {
  std::string container = "wav";  // M2 supports "wav"; more formats in M4
  int bitDepth = 16;              // integer PCM bit depth
  bool floatFormat = false;       // if true, write 32-bit IEEE float
};

// Encodes `buffer` to `path` per `spec`. Throws AudioError on unsupported
// container or I/O failure.
void encodeToFile(const std::string& path, const PcmBuffer& buffer,
                  const EncodeSpec& spec);

}  // namespace djcore
