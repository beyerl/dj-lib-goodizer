#include "djcore/analysis/analyzers/StereoWidthAnalyzer.h"

#include <cmath>

namespace djcore {

void StereoWidthAnalyzer::begin(const FormatInfo& format) {
  channels_ = format.channels;
  midPow_ = sidePow_ = sumL2_ = sumR2_ = 0.0;
}

void StereoWidthAnalyzer::processBlock(const PcmBlock& block) {
  if (block.channels < 2) return;
  const float* L = block.planes[0];
  const float* R = block.planes[1];
  for (std::size_t i = 0; i < block.frames; ++i) {
    const double l = L[i];
    const double r = R[i];
    const double mid = (l + r) * 0.5;
    const double side = (l - r) * 0.5;
    midPow_ += mid * mid;
    sidePow_ += side * side;
    sumL2_ += l * l;
    sumR2_ += r * r;
  }
}

void StereoWidthAnalyzer::finalize(AnalysisResult& result) {
  if (channels_ < 2) return;  // leave optionals unset for mono
  const double total = midPow_ + sidePow_;
  result.stereoWidth = total > 0.0 ? sidePow_ / total : 0.0;
  result.lrBalanceDb =
      (sumL2_ > 0.0 && sumR2_ > 0.0) ? 10.0 * std::log10(sumR2_ / sumL2_) : 0.0;
}

}  // namespace djcore
