#pragma once

#include <memory>
#include <string>
#include <vector>

#include "djcore/analysis/AnalysisConfig.h"
#include "djcore/analysis/IAnalyzer.h"

namespace djcore {

class AudioDecoder;

// Decodes a file once and fans streaming PCM blocks to every registered
// analyzer, producing a single combined AnalysisResult. By default it registers
// all four analyzers (silence, loudness/dynamics, phase/mono, stereo width).
class AnalysisPipeline {
 public:
  explicit AnalysisPipeline(const AnalysisConfig& config = {});

  void addAnalyzer(std::unique_ptr<IAnalyzer> analyzer);

  AnalysisResult analyze(AudioDecoder& decoder);
  AnalysisResult analyzeFile(const std::string& path);

  static constexpr int kAnalyzerVersion = 1;

 private:
  std::vector<std::unique_ptr<IAnalyzer>> analyzers_;
};

}  // namespace djcore
