#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "djcore/jobs/JobControl.h"
#include "djcore/model/AnalysisResult.h"

namespace djcore {

struct AnalysisBatchItem {
  std::string path;
  std::optional<AnalysisResult> result;
  bool ok = false;
  std::string error;
};

struct BatchProgress {
  std::size_t total = 0;
  std::size_t completed = 0;
  std::size_t failed = 0;
};

using BatchProgressCallback = std::function<void(const BatchProgress&)>;

// Analyzes many files in parallel (each worker runs its own AnalysisPipeline),
// with cooperative pause/resume/cancel and progress reporting.
class AnalysisBatch {
 public:
  std::vector<AnalysisBatchItem> run(const std::vector<std::string>& paths,
                                     JobControl& control,
                                     const BatchProgressCallback& onProgress = {},
                                     unsigned threads = 0);
};

}  // namespace djcore
