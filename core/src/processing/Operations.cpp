#include "djcore/processing/Operations.h"

#include <algorithm>
#include <cmath>

namespace djcore {
namespace {

// Index within +/- window of `idx` whose summed cross-channel magnitude is
// smallest (closest to a zero crossing).
std::size_t snapToZeroCrossing(const PcmBuffer& b, std::size_t idx,
                               std::size_t window) {
  const std::size_t lo = idx > window ? idx - window : 0;
  const std::size_t hi = std::min(b.frames(), idx + window + 1);
  std::size_t best = idx;
  double bestMag = 1e30;
  for (std::size_t i = lo; i < hi; ++i) {
    double m = 0.0;
    for (int c = 0; c < b.channels(); ++c) m += std::fabs(b.channel(c)[i]);
    if (m < bestMag) {
      bestMag = m;
      best = i;
    }
  }
  return best;
}

}  // namespace

void GainOperation::apply(PcmBuffer& buffer) {
  for (int c = 0; c < buffer.channels(); ++c) {
    float* p = buffer.channel(c);
    for (std::size_t i = 0; i < buffer.frames(); ++i) {
      p[i] = static_cast<float>(p[i] * gain_);
    }
  }
}

PcmBuffer trimSilence(const PcmBuffer& in, std::size_t startFrames,
                      std::size_t endFrames) {
  if (in.empty()) return in;
  const std::size_t window =
      in.sampleRate() > 0 ? static_cast<std::size_t>(in.sampleRate() / 1000) : 16;

  const std::size_t start =
      snapToZeroCrossing(in, std::min(startFrames, in.frames()), window);
  const std::size_t endCut = std::min(endFrames, in.frames() - start);
  std::size_t lastKeep = in.frames() - endCut;
  if (lastKeep > start) lastKeep = snapToZeroCrossing(in, lastKeep, window);
  const std::size_t outFrames = lastKeep > start ? lastKeep - start : 0;

  PcmBuffer out(in.channels(), in.sampleRate(), outFrames);
  for (int c = 0; c < in.channels(); ++c) {
    const float* s = in.channel(c);
    float* d = out.channel(c);
    for (std::size_t i = 0; i < outFrames; ++i) d[i] = s[start + i];
  }
  return out;
}

}  // namespace djcore
