#include "djcore/jobs/AnalysisBatch.h"

#include <chrono>
#include <exception>

#include "djcore/analysis/AnalysisPipeline.h"

namespace djcore {

std::vector<AnalysisBatchItem> AnalysisBatch::run(
    const std::vector<std::string>& paths, JobControl& control,
    const BatchProgressCallback& onProgress, unsigned threads) {
  std::vector<AnalysisBatchItem> items(paths.size());
  for (std::size_t i = 0; i < paths.size(); ++i) items[i].path = paths[i];

  const auto start = std::chrono::steady_clock::now();
  const AnalysisConfig cfg = cfg_;

  auto worker = [&](std::size_t i) -> bool {
    // Each task constructs its own pipeline => analyzers are not shared across
    // threads (they are stateful accumulators).
    AnalysisPipeline pipeline(cfg);
    try {
      AnalysisResult r = pipeline.analyzeFile(items[i].path);
      items[i].result = r;
      return true;
    } catch (const std::exception& e) {
      items[i].error = e.what();
      return false;
    }
  };

  ProgressCallback progress;
  if (onProgress) {
    progress = [&](std::size_t completed, std::size_t failed) {
      const double elapsed =
          std::chrono::duration<double>(std::chrono::steady_clock::now() - start)
              .count();
      BatchProgress bp;
      bp.total = paths.size();
      bp.completed = completed;
      bp.failed = failed;
      bp.elapsedSeconds = elapsed;
      bp.etaSeconds = completed > 0
                          ? elapsed / completed * (paths.size() - completed)
                          : 0.0;
      onProgress(bp);
    };
  }

  parallelFor(paths.size(), worker, control, progress, threads);
  return items;
}

}  // namespace djcore
