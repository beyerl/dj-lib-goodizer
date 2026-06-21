#include "djcore/processing/PerceptualOps.h"

#include <algorithm>
#include <cmath>

namespace djcore {

void CompressionOperation::apply(PcmBuffer& buffer) {
  const double sr = buffer.sampleRate() > 0 ? buffer.sampleRate() : 44100.0;
  const double threshLin = std::pow(10.0, s_.thresholdDb / 20.0);
  const double makeup = std::pow(10.0, s_.makeupDb / 20.0);
  const double attackCoef = std::exp(-1.0 / (s_.attackMs * 0.001 * sr));
  const double releaseCoef = std::exp(-1.0 / (s_.releaseMs * 0.001 * sr));

  double env = 0.0;
  for (std::size_t i = 0; i < buffer.frames(); ++i) {
    double peak = 0.0;
    for (int ch = 0; ch < buffer.channels(); ++ch) {
      peak = std::max(peak, std::abs(static_cast<double>(buffer.channel(ch)[i])));
    }
    const double coef = peak > env ? attackCoef : releaseCoef;
    env = coef * env + (1.0 - coef) * peak;

    double gain = 1.0;
    if (env > threshLin && env > 0.0) {
      const double overDb = 20.0 * std::log10(env) - s_.thresholdDb;
      const double reductionDb = overDb * (1.0 / s_.ratio - 1.0);  // <= 0
      gain = std::pow(10.0, reductionDb / 20.0);
    }
    const double g = gain * makeup;
    for (int ch = 0; ch < buffer.channels(); ++ch) {
      buffer.channel(ch)[i] = static_cast<float>(buffer.channel(ch)[i] * g);
    }
  }
}

void WidthBalanceOperation::apply(PcmBuffer& buffer) {
  if (buffer.channels() < 2) return;  // width/balance only meaningful in stereo
  float* l = buffer.channel(0);
  float* r = buffer.channel(1);
  // A correction of c dB shifts the R-L balance by c dB: boost left / cut right
  // by half each for c < 0 (corrects a right-heavy skew), and vice versa.
  const double gl = std::pow(10.0, (-balanceCorrectionDb_ * 0.5) / 20.0);
  const double gr = std::pow(10.0, (balanceCorrectionDb_ * 0.5) / 20.0);
  for (std::size_t i = 0; i < buffer.frames(); ++i) {
    const double lv = l[i] * gl;
    const double rv = r[i] * gr;
    const double mid = 0.5 * (lv + rv);
    const double side = 0.5 * (lv - rv) * widthFactor_;
    l[i] = static_cast<float>(mid + side);
    r[i] = static_cast<float>(mid - side);
  }
}

double predictMonoCorrelationAfterWidth(const PcmBuffer& buffer, double widthFactor) {
  if (buffer.channels() < 2) return 1.0;
  const float* l = buffer.channel(0);
  const float* r = buffer.channel(1);
  double sumLR = 0.0, sumL2 = 0.0, sumR2 = 0.0;
  for (std::size_t i = 0; i < buffer.frames(); ++i) {
    const double mid = 0.5 * (l[i] + r[i]);
    const double side = 0.5 * (l[i] - r[i]) * widthFactor;
    const double nl = mid + side;
    const double nr = mid - side;
    sumLR += nl * nr;
    sumL2 += nl * nl;
    sumR2 += nr * nr;
  }
  const double denom = std::sqrt(sumL2 * sumR2);
  return denom > 0.0 ? sumLR / denom : 1.0;
}

}  // namespace djcore
