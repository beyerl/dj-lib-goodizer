#pragma once

#include "djcore/analysis/IAnalyzer.h"

namespace djcore {

// Stereo width and L/R balance for stereo tracks (FR-WID-1/2). Width is derived
// from the mid/side energy split on a 0..1 scale (0 = mono-centred, 1 = fully
// "wide"/anti-phase). Balance is the inter-channel level skew in dB (+ve =>
// right louder). Mono/non-stereo tracks leave these metrics unset.
class StereoWidthAnalyzer : public IAnalyzer {
 public:
  const char* name() const override { return "stereo-width"; }
  void begin(const FormatInfo& format) override;
  void processBlock(const PcmBlock& block) override;
  void finalize(AnalysisResult& result) override;

 private:
  bool stereo_ = false;
  double midPower_ = 0.0;
  double sidePower_ = 0.0;
  double sumL2_ = 0.0;
  double sumR2_ = 0.0;
};

}  // namespace djcore
