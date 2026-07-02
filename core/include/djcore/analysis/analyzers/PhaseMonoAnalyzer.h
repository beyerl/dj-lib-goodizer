#pragma once

#include "djcore/analysis/IAnalyzer.h"

namespace djcore {

// Inter-channel phase correlation and mono fold-down level delta. Stereo only;
// leaves the (optional) result fields unset for mono files.
class PhaseMonoAnalyzer : public IAnalyzer {
 public:
  const char* name() const override { return "phase_mono"; }
  void begin(const FormatInfo& format) override;
  void processBlock(const PcmBlock& block) override;
  void finalize(AnalysisResult& result) override;

 private:
  int channels_ = 0;
  double sumLR_ = 0.0;
  double sumL2_ = 0.0;
  double sumR2_ = 0.0;
  double sumMono2_ = 0.0;  // energy of (L+R)/2
};

}  // namespace djcore
