#include "djcore/audio/PcmBuffer.h"

namespace djcore {

PcmBuffer::PcmBuffer(int channels, int sampleRate, std::size_t frames)
    : channels_(channels),
      sampleRate_(sampleRate),
      frames_(frames),
      planes_(channels > 0 ? static_cast<std::size_t>(channels) : 0) {
  for (auto& plane : planes_) {
    plane.assign(frames_, 0.0f);
  }
}

void PcmBuffer::resize(std::size_t frames) {
  frames_ = frames;
  for (auto& plane : planes_) {
    plane.resize(frames_, 0.0f);
  }
}

float* PcmBuffer::channel(int c) {
  return planes_[static_cast<std::size_t>(c)].data();
}

const float* PcmBuffer::channel(int c) const {
  return planes_[static_cast<std::size_t>(c)].data();
}

}  // namespace djcore
