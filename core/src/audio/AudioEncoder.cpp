#include "djcore/audio/AudioEncoder.h"

#include "djcore/audio/AudioError.h"
#include "djcore/audio/WavIo.h"

namespace djcore {

void encodeToFile(const std::string& path, const PcmBuffer& buffer,
                  const EncodeSpec& spec) {
  if (spec.container == "wav") {
    writeWav(path, buffer, spec.bitDepth, spec.floatFormat, spec.dither);
    return;
  }
  // FLAC/AIFF/lossy encoders arrive with the FFmpeg backend in M4.
  throw AudioError("unsupported output container: " + spec.container);
}

}  // namespace djcore
