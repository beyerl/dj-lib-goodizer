#include "djcore/analysis/analyzers/StereoWidthAnalyzer.h"

#include <cmath>

namespace djcore {

void StereoWidthAnalyzer::begin(const FormatInfo& format) {
  stereo_ = (format.channels == 2);
  midPower_ = sidePower_ = sumL2_ = sumR2_ = 0.0;
}

void StereoWidthAnalyzer::processBlock(const PcmBlock& block) {
  if (!stereo_ || block.channels < 2) {
    return;
  }
  const float* l = block.planes[0];
  const float* r = block.planes[1];
  for (std::size_t i = 0; i < block.frames; ++i) {
    const double lv = l[i];
    const double rv = r[i];
    const double mid = 0.5 * (lv + rv);
    const double side = 0.5 * (lv - rv);
    midPower_ += mid * mid;
    sidePower_ += side * side;
    sumL2_ += lv * lv;
    sumR2_ += rv * rv;
  }
}

void StereoWidthAnalyzer::finalize(AnalysisResult& result) {
  if (!stereo_) {
    return;
  }
  const double total = midPower_ + sidePower_;
  result.stereoWidth = total > 0.0 ? sidePower_ / total : 0.0;

  // L/R balance in dB: +ve means the right channel is louder.
  if (sumL2_ > 0.0 && sumR2_ > 0.0) {
    result.lrBalanceDb = 10.0 * std::log10(sumR2_ / sumL2_);
  } else {
    result.lrBalanceDb = 0.0;
  }
}

}  // namespace djcore
