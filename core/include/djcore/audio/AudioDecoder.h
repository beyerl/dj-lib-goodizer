#pragma once

#include <memory>
#include <string>

#include "djcore/audio/FormatInfo.h"
#include "djcore/audio/PcmBuffer.h"

namespace djcore {

// Abstraction over decoding a container to canonical planar float32 PCM.
// M2 ships a WAV backend; the FFmpeg backend (broad formats) is added in M4
// behind DJ_WITH_AUDIO_BACKENDS and slots into openDecoder() below.
class AudioDecoder {
 public:
  virtual ~AudioDecoder() = default;
  virtual const FormatInfo& format() const = 0;
  virtual PcmBuffer readAll() = 0;
};

// Probes the file and returns a decoder for it. Throws AudioError if no backend
// can handle the file. Also usable to cheaply read just the FormatInfo via
// probeFormat().
std::unique_ptr<AudioDecoder> openDecoder(const std::string& path);

// Reads only the format metadata (no full decode where the backend allows it).
FormatInfo probeFormat(const std::string& path);

// True if openDecoder() has a backend for this file's extension/contents.
bool canDecode(const std::string& path);

}  // namespace djcore
