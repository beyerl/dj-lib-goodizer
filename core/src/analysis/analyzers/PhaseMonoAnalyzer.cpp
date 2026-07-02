#include "djcore/analysis/analyzers/PhaseMonoAnalyzer.h"

#include <cmath>

namespace djcore {

void PhaseMonoAnalyzer::begin(const FormatInfo& format) {
  channels_ = format.channels;
  sumLR_ = sumL2_ = sumR2_ = sumMono2_ = 0.0;
}

void PhaseMonoAnalyzer::processBlock(const PcmBlock& block) {
  if (block.channels < 2) return;
  const float* L = block.planes[0];
  const float* R = block.planes[1];
  for (std::size_t i = 0; i < block.frames; ++i) {
    const double l = L[i];
    const double r = R[i];
    sumLR_ += l * r;
    sumL2_ += l * l;
    sumR2_ += r * r;
    const double m = (l + r) * 0.5;
    sumMono2_ += m * m;
  }
}

void PhaseMonoAnalyzer::finalize(AnalysisResult& result) {
  if (channels_ < 2) return;  // leave optionals unset for mono

  const double denom = std::sqrt(sumL2_ * sumR2_);
  double rho = denom > 0.0 ? sumLR_ / denom : 1.0;  // silence is mono-safe
  if (rho > 1.0) rho = 1.0;
  if (rho < -1.0) rho = -1.0;
  result.phaseCorrelation = rho;

  const double ref = 0.5 * (sumL2_ + sumR2_);
  double delta;
  if (ref <= 0.0)
    delta = 0.0;
  else if (sumMono2_ <= 0.0)
    delta = -120.0;  // full cancellation
  else
    delta = 10.0 * std::log10(sumMono2_ / ref);
  result.monoFolddownDeltaDb = delta;
}

}  // namespace djcore
