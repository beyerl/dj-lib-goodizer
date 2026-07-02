#pragma once

#include "djcore/analysis/AnalysisConfig.h"
#include "djcore/analysis/IAnalyzer.h"

namespace djcore {

// Leading/trailing silence boundaries with a configurable threshold and a
// minimum-duration gate; flags ambiguous quiet intros (sustained low-level
// content) rather than treating them as trimmable silence.
class SilenceDetector : public IAnalyzer {
 public:
  explicit SilenceDetector(const AnalysisConfig& cfg = {}) : cfg_(cfg) {}
  const char* name() const override { return "silence"; }
  void begin(const FormatInfo& format) override;
  void processBlock(const PcmBlock& block) override;
  void finalize(AnalysisResult& result) override;

 private:
  AnalysisConfig cfg_;
  int sampleRate_ = 0;
  double threshold_ = 0.0;   // linear silence threshold
  double ambiguous_ = 0.0;   // linear "quiet but present" threshold
  long long firstLoud_ = -1;
  long long lastLoud_ = -1;
  long long totalFrames_ = 0;
  long long quietFrames_ = 0;
};

}  // namespace djcore
