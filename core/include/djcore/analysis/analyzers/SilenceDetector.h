#pragma once

#include <cstdint>

#include "djcore/analysis/AnalysisConfig.h"
#include "djcore/analysis/IAnalyzer.h"

namespace djcore {

// Detects leading/trailing silence (FR-SIL-1/2) and flags ambiguous low-level
// intros/outros that should be reviewed rather than auto-trimmed (FR-SIL-7).
class SilenceDetector : public IAnalyzer {
 public:
  explicit SilenceDetector(const AnalysisConfig& cfg = {});

  const char* name() const override { return "silence"; }
  void begin(const FormatInfo& format) override;
  void processBlock(const PcmBlock& block) override;
  void finalize(AnalysisResult& result) override;

 private:
  double thresholdLinear_ = 0.001;
  double ambiguousLinear_ = 0.01;  // a higher "quiet content" band (-40 dB)
  int sampleRate_ = 0;
  std::int64_t totalFrames_ = 0;
  std::int64_t firstLoud_ = -1;
  std::int64_t lastLoud_ = -1;
  std::int64_t quietContentFrames_ = 0;  // frames in the ambiguous band
};

}  // namespace djcore
