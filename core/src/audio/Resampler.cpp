#include "djcore/audio/Resampler.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace djcore {
namespace {

constexpr double kPi = 3.14159265358979323846;

double besselI0(double x) {
  double sum = 1.0;
  double term = 1.0;
  const double xx = x * x / 4.0;
  for (int k = 1; k < 60; ++k) {
    term *= xx / (static_cast<double>(k) * k);
    sum += term;
    if (term < 1e-12 * sum) break;
  }
  return sum;
}

double sinc(double x) {
  if (std::fabs(x) < 1e-9) return 1.0;
  const double px = kPi * x;
  return std::sin(px) / px;
}

}  // namespace

PcmBuffer resample(const PcmBuffer& in, int dstSampleRate, int quality) {
  if (in.empty() || in.sampleRate() <= 0 || dstSampleRate <= 0 ||
      in.sampleRate() == dstSampleRate) {
    PcmBuffer copy = in;
    if (dstSampleRate > 0) copy.setSampleRate(dstSampleRate);
    return copy;
  }

  const int srcRate = in.sampleRate();
  const int channels = in.channels();
  const std::size_t nIn = in.frames();
  const double ratio = static_cast<double>(dstSampleRate) / srcRate;
  const std::size_t nOut = static_cast<std::size_t>(std::ceil(nIn * ratio));

  const double cutoff = 0.5 * std::min(1.0, ratio);   // cycles/source-sample
  const double halfWidth = quality / (2.0 * cutoff);  // in source samples
  const double beta = 9.0;                            // Kaiser: ~ -90 dB sidelobes

  // Precompute the (symmetric) windowed-sinc kernel ONCE into a dense table, so
  // the per-output-sample inner loop is a cheap lookup instead of evaluating
  // sin()/besselI0() per tap (which made real-length resampling take seconds
  // per track — the "standardize hangs" bug).
  constexpr int kDensity = 512;  // table samples per source-sample
  const std::size_t tableSize =
      static_cast<std::size_t>(std::ceil(halfWidth * kDensity)) + 2;
  std::vector<double> table(tableSize);
  const double invI0Beta = 1.0 / besselI0(beta);
  for (std::size_t i = 0; i < tableSize; ++i) {
    const double x = static_cast<double>(i) / kDensity;  // distance, source samples
    const double t = x / halfWidth;                      // 0 .. ~1
    const double win =
        t < 1.0 ? besselI0(beta * std::sqrt(std::max(0.0, 1.0 - t * t))) * invI0Beta
                : 0.0;
    table[i] = 2.0 * cutoff * sinc(2.0 * cutoff * x) * win;
  }
  auto kernel = [&](double dxAbs) -> double {
    const double pos = dxAbs * kDensity;
    const std::size_t i = static_cast<std::size_t>(pos);
    if (i + 1 >= tableSize) return 0.0;
    const double f = pos - static_cast<double>(i);
    return table[i] * (1.0 - f) + table[i + 1] * f;  // linear interp of the table
  };

  PcmBuffer out(channels, dstSampleRate, nOut);
  for (int c = 0; c < channels; ++c) {
    const float* s = in.channel(c);
    float* d = out.channel(c);
    for (std::size_t i = 0; i < nOut; ++i) {
      const double center = static_cast<double>(i) / ratio;  // in source samples
      const long n0 = static_cast<long>(std::ceil(center - halfWidth));
      const long n1 = static_cast<long>(std::floor(center + halfWidth));
      double acc = 0.0;
      for (long n = n0; n <= n1; ++n) {
        if (n < 0 || n >= static_cast<long>(nIn)) continue;
        acc += s[n] * kernel(std::fabs(center - n));
      }
      d[i] = static_cast<float>(acc);
    }
  }
  return out;
}

}  // namespace djcore
