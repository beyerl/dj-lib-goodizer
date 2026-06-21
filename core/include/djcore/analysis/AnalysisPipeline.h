#pragma once

#include <memory>
#include <string>
#include <vector>

#include "djcore/analysis/AnalysisConfig.h"
#include "djcore/analysis/IAnalyzer.h"
#include "djcore/audio/AudioDecoder.h"
#include "djcore/model/AnalysisResult.h"

namespace djcore {

// Decodes a track once and fans each streamed PCM block to every registered
// analyzer (plan §2 — no file is decoded more than once). The default set
// covers all stereo/loudness/silence feature areas.
class AnalysisPipeline {
 public:
  explicit AnalysisPipeline(const AnalysisConfig& cfg = {});

  void addAnalyzer(std::unique_ptr<IAnalyzer> analyzer);

  // Run all analyzers over an already-open decoder.
  AnalysisResult analyze(AudioDecoder& decoder);

  // Convenience: open the file with the best backend and analyze it.
  AnalysisResult analyzeFile(const std::string& path);

 private:
  std::vector<std::unique_ptr<IAnalyzer>> analyzers_;
};

}  // namespace djcore
