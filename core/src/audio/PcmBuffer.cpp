#include "djcore/audio/PcmBuffer.h"

namespace djcore {

PcmBuffer::PcmBuffer(int channels, int sampleRate, std::size_t frames)
    : channels_(channels),
      sampleRate_(sampleRate),
      frames_(frames),
      planes_(static_cast<std::size_t>(channels < 0 ? 0 : channels)) {
  for (auto& plane : planes_) {
    plane.assign(frames, 0.0f);
  }
}

void PcmBuffer::resize(std::size_t frames) {
  frames_ = frames;
  for (auto& plane : planes_) {
    plane.assign(frames, 0.0f);
  }
}

}  // namespace djcore
