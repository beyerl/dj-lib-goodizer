#pragma once

#include <string>

#include "djcore/audio/PcmBuffer.h"

namespace djcore {

// Target encoding parameters for an output file.
struct EncodeSpec {
  std::string container = "wav";  // "wav" (dependency-free); others via FFmpeg
  int sampleRate = 44100;
  int bitDepth = 16;              // 16/24/32-int, or 32 for float when isFloat
  bool isFloat = false;           // true => 32-bit IEEE float samples
};

// Writes a float32 planar PcmBuffer to disk in the requested format.
// The dependency-free implementation supports WAV; broad-format encoding is
// added behind DJ_WITH_AUDIO_BACKENDS. Throws AudioError on failure.
void encodeToFile(const std::string& path, const PcmBuffer& pcm,
                  const EncodeSpec& spec);

}  // namespace djcore
