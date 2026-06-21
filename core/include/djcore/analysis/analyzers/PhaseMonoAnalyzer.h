#pragma once

#include "djcore/analysis/IAnalyzer.h"

namespace djcore {

// Mono-compatibility analysis for stereo tracks (FR-MONO-1/2): inter-channel
// phase correlation coefficient and the level loss incurred when the signal is
// summed to mono. Mono/non-stereo tracks leave these metrics unset.
class PhaseMonoAnalyzer : public IAnalyzer {
 public:
  const char* name() const override { return "phase-mono"; }
  void begin(const FormatInfo& format) override;
  void processBlock(const PcmBlock& block) override;
  void finalize(AnalysisResult& result) override;

 private:
  bool stereo_ = false;
  double sumLR_ = 0.0;
  double sumL2_ = 0.0;
  double sumR2_ = 0.0;
  double sumMono2_ = 0.0;  // power of (L+R)/2
};

}  // namespace djcore
