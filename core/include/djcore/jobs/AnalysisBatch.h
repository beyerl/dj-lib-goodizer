#pragma once

#include <optional>
#include <string>
#include <vector>

#include "djcore/analysis/AnalysisConfig.h"
#include "djcore/jobs/JobControl.h"
#include "djcore/jobs/ParallelFor.h"
#include "djcore/model/AnalysisResult.h"

namespace djcore {

struct AnalysisBatchItem {
  std::string path;
  std::optional<AnalysisResult> result;  // unset if analysis failed
  std::string error;
};

// Overall batch progress with a rough ETA (NFR-PERF-4).
struct BatchProgress {
  std::size_t total = 0;
  std::size_t completed = 0;
  std::size_t failed = 0;
  double elapsedSeconds = 0.0;
  double etaSeconds = 0.0;
};

using BatchProgressCallback = std::function<void(const BatchProgress&)>;

// Runs the analysis pipeline over many files in parallel, off the UI thread,
// with pause/resume/cancel and progress reporting (plan §5).
class AnalysisBatch {
 public:
  explicit AnalysisBatch(const AnalysisConfig& cfg = {}) : cfg_(cfg) {}

  std::vector<AnalysisBatchItem> run(const std::vector<std::string>& paths,
                                     JobControl& control,
                                     const BatchProgressCallback& onProgress = {},
                                     unsigned threads = 0);

 private:
  AnalysisConfig cfg_;
};

}  // namespace djcore
