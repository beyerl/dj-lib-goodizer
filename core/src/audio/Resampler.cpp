#include "djcore/audio/Resampler.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace djcore {
namespace {

constexpr double kPi = 3.14159265358979323846;

double sinc(double x) {
  if (std::abs(x) < 1e-9) {
    return 1.0;
  }
  const double px = kPi * x;
  return std::sin(px) / px;
}

// Blackman window over [-1, 1] (argument already normalized to half-width).
double blackman(double t) {
  if (t < -1.0 || t > 1.0) {
    return 0.0;
  }
  const double w = (t + 1.0) * 0.5;  // map [-1,1] -> [0,1]
  return 0.42 - 0.5 * std::cos(2.0 * kPi * w) + 0.08 * std::cos(4.0 * kPi * w);
}

}  // namespace

PcmBuffer resample(const PcmBuffer& input, int dstSampleRate, int quality) {
  const int srcRate = input.sampleRate();
  if (srcRate <= 0 || dstSampleRate <= 0 || input.frames() == 0) {
    return PcmBuffer(input.channels(), dstSampleRate, 0);
  }
  if (srcRate == dstSampleRate) {
    return input;  // nothing to do
  }

  const double ratio = static_cast<double>(dstSampleRate) / srcRate;
  const std::size_t srcFrames = input.frames();
  const std::size_t dstFrames =
      static_cast<std::size_t>(std::ceil(srcFrames * ratio));

  // Cutoff (cycles per source sample): 0.5 at/above unity, lowered when
  // downsampling so the kernel band-limits to the destination Nyquist.
  const double cutoff = 0.5 * std::min(1.0, ratio);
  // Kernel half-width in source samples. More zero crossings near the cutoff
  // means a sharper filter; scale by 1/(2*cutoff) so downsampling widens it.
  const double halfWidth = static_cast<double>(quality) / (2.0 * cutoff);

  PcmBuffer out(input.channels(), dstSampleRate, dstFrames);

  for (int ch = 0; ch < input.channels(); ++ch) {
    const float* in = input.channel(ch);
    float* dst = out.channel(ch);

    for (std::size_t i = 0; i < dstFrames; ++i) {
      // Position in source-sample units.
      const double center = static_cast<double>(i) / ratio;
      const long first =
          static_cast<long>(std::ceil(center - halfWidth));
      const long last = static_cast<long>(std::floor(center + halfWidth));

      double acc = 0.0;
      for (long n = first; n <= last; ++n) {
        if (n < 0 || n >= static_cast<long>(srcFrames)) {
          continue;
        }
        const double d = center - static_cast<double>(n);
        const double kernel =
            2.0 * cutoff * sinc(2.0 * cutoff * d) * blackman(d / halfWidth);
        acc += static_cast<double>(in[n]) * kernel;
      }
      dst[i] = static_cast<float>(acc);
    }
  }
  return out;
}

}  // namespace djcore
