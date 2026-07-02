#pragma once

#include "djcore/analysis/IAnalyzer.h"

namespace djcore {

// Mid/side stereo width (0=mono .. 1=all-side) and L/R balance (dB, +ve = right
// louder). Stereo only; leaves the (optional) result fields unset for mono.
class StereoWidthAnalyzer : public IAnalyzer {
 public:
  const char* name() const override { return "stereo_width"; }
  void begin(const FormatInfo& format) override;
  void processBlock(const PcmBlock& block) override;
  void finalize(AnalysisResult& result) override;

 private:
  int channels_ = 0;
  double midPow_ = 0.0;
  double sidePow_ = 0.0;
  double sumL2_ = 0.0;
  double sumR2_ = 0.0;
};

}  // namespace djcore
