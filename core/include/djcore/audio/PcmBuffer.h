#pragma once

#include <cstddef>
#include <vector>

namespace djcore {

// Canonical internal PCM representation (plan §1): 32-bit float, planar —
// one contiguous vector of samples per channel. Every analyzer and operation
// works on this representation regardless of the source/target codec.
class PcmBuffer {
 public:
  PcmBuffer() = default;
  PcmBuffer(int channels, int sampleRate, std::size_t frames = 0);

  int channels() const { return channels_; }
  int sampleRate() const { return sampleRate_; }
  std::size_t frames() const { return frames_; }

  float* channel(int c) { return planes_[static_cast<std::size_t>(c)].data(); }
  const float* channel(int c) const {
    return planes_[static_cast<std::size_t>(c)].data();
  }

  void resize(std::size_t frames);

 private:
  int channels_ = 0;
  int sampleRate_ = 0;
  std::size_t frames_ = 0;
  std::vector<std::vector<float>> planes_;  // [channel][frame]
};

// A non-owning view over a block of planar PCM, handed to analyzers during the
// single-decode streaming fan-out (plan §2).
struct PcmBlock {
  const float* const* planes = nullptr;  // planes[channel][frame]
  int channels = 0;
  int sampleRate = 0;
  std::size_t frames = 0;
};

}  // namespace djcore
