#pragma once

#include <cstddef>
#include <vector>

namespace djcore {

// Canonical internal audio representation: planar, 32-bit float PCM.
// One contiguous vector per channel (planes_[channel][frame]). Sample values
// are nominally in [-1, +1] but may exceed it (clipping / inter-sample
// overshoot) and must never be pre-clamped by the buffer itself.
class PcmBuffer {
 public:
  PcmBuffer() = default;
  PcmBuffer(int channels, int sampleRate, std::size_t frames);

  int channels() const { return channels_; }
  int sampleRate() const { return sampleRate_; }
  std::size_t frames() const { return frames_; }
  bool empty() const { return channels_ == 0 || frames_ == 0; }

  void setSampleRate(int sampleRate) { sampleRate_ = sampleRate; }

  // Resize the frame count of every channel, preserving existing samples where
  // the buffers overlap and zero-filling any growth.
  void resize(std::size_t frames);

  // Access a channel's contiguous sample plane. Precondition: 0 <= c < channels().
  float* channel(int c);
  const float* channel(int c) const;

 private:
  int channels_ = 0;
  int sampleRate_ = 0;
  std::size_t frames_ = 0;
  std::vector<std::vector<float>> planes_;  // [channel][frame]
};

// Non-owning, read-only view of a contiguous run of frames across all channels,
// used to stream decoded audio to analyzers. planes[c][i] is frame i of channel c.
struct PcmBlock {
  const float* const* planes = nullptr;
  int channels = 0;
  int sampleRate = 0;
  std::size_t frames = 0;
};

}  // namespace djcore
