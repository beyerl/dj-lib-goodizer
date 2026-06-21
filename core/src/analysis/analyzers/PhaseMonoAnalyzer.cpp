#include "djcore/analysis/analyzers/PhaseMonoAnalyzer.h"

#include <cmath>

namespace djcore {

void PhaseMonoAnalyzer::begin(const FormatInfo& format) {
  stereo_ = (format.channels == 2);
  sumLR_ = sumL2_ = sumR2_ = sumMono2_ = 0.0;
}

void PhaseMonoAnalyzer::processBlock(const PcmBlock& block) {
  if (!stereo_ || block.channels < 2) {
    return;
  }
  const float* l = block.planes[0];
  const float* r = block.planes[1];
  for (std::size_t i = 0; i < block.frames; ++i) {
    const double lv = l[i];
    const double rv = r[i];
    sumLR_ += lv * rv;
    sumL2_ += lv * lv;
    sumR2_ += rv * rv;
    const double mono = 0.5 * (lv + rv);
    sumMono2_ += mono * mono;
  }
}

void PhaseMonoAnalyzer::finalize(AnalysisResult& result) {
  if (!stereo_) {
    return;
  }
  // Pearson correlation between channels: +1 fully in-phase, -1 fully inverted.
  const double denom = std::sqrt(sumL2_ * sumR2_);
  result.phaseCorrelation = denom > 0.0 ? sumLR_ / denom : 1.0;

  // Mono fold-down level delta: power of (L+R)/2 vs the mean channel power.
  // 0 dB => no loss; large negative => significant cancellation on mono sum.
  const double refPower = 0.5 * (sumL2_ + sumR2_);
  if (refPower > 0.0 && sumMono2_ > 0.0) {
    result.monoFolddownDeltaDb = 10.0 * std::log10(sumMono2_ / refPower);
  } else if (refPower > 0.0) {
    result.monoFolddownDeltaDb = -120.0;  // total cancellation
  }
}

}  // namespace djcore
