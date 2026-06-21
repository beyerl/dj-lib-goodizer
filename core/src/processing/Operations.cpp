#include "djcore/processing/Operations.h"

#include <algorithm>
#include <cmath>

namespace djcore {

void GainOperation::apply(PcmBuffer& buffer) {
  const float g = static_cast<float>(gain_);
  for (int ch = 0; ch < buffer.channels(); ++ch) {
    float* p = buffer.channel(ch);
    for (std::size_t i = 0; i < buffer.frames(); ++i) {
      p[i] *= g;
    }
  }
}

namespace {

// Snap a cut index toward the nearest zero crossing (summed across channels)
// within +/- window frames, so the edit lands where the waveform is ~0.
std::size_t snapToZeroCrossing(const PcmBuffer& in, std::size_t idx,
                               std::size_t window) {
  const std::size_t n = in.frames();
  if (idx >= n) return n;
  const std::size_t lo = idx > window ? idx - window : 0;
  const std::size_t hi = std::min(n - 1, idx + window);
  std::size_t best = idx;
  double bestMag = 1e30;
  for (std::size_t i = lo; i <= hi; ++i) {
    double mag = 0.0;
    for (int ch = 0; ch < in.channels(); ++ch) {
      mag += std::abs(static_cast<double>(in.channel(ch)[i]));
    }
    if (mag < bestMag) {
      bestMag = mag;
      best = i;
    }
  }
  return best;
}

}  // namespace

PcmBuffer trimSilence(const PcmBuffer& in, std::size_t startFrames,
                      std::size_t endFrames) {
  if (in.frames() == 0) return in;

  const std::size_t window = static_cast<std::size_t>(in.sampleRate()) / 1000;  // ~1 ms
  std::size_t start = std::min(startFrames, in.frames());
  start = snapToZeroCrossing(in, start, window);

  std::size_t endCut = std::min(endFrames, in.frames() - start);
  std::size_t lastKeep = in.frames() - endCut;  // exclusive
  if (lastKeep > start) {
    lastKeep = snapToZeroCrossing(in, lastKeep, window);
  }

  const std::size_t outFrames = lastKeep > start ? lastKeep - start : 0;
  PcmBuffer out(in.channels(), in.sampleRate(), outFrames);
  for (int ch = 0; ch < in.channels(); ++ch) {
    const float* src = in.channel(ch) + start;
    std::copy(src, src + outFrames, out.channel(ch));
  }
  return out;
}

}  // namespace djcore
