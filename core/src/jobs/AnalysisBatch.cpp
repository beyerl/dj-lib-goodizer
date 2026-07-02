#include "djcore/jobs/AnalysisBatch.h"

#include <exception>

#include "djcore/analysis/AnalysisPipeline.h"
#include "djcore/jobs/ParallelFor.h"

namespace djcore {

std::vector<AnalysisBatchItem> AnalysisBatch::run(
    const std::vector<std::string>& paths, JobControl& control,
    const BatchProgressCallback& onProgress, unsigned threads) {
  std::vector<AnalysisBatchItem> items(paths.size());
  for (std::size_t i = 0; i < paths.size(); ++i) items[i].path = paths[i];

  const std::size_t total = paths.size();
  parallelFor(
      total,
      [&](std::size_t i) -> bool {
        try {
          AnalysisPipeline pipeline;  // per-item state
          items[i].result = pipeline.analyzeFile(items[i].path);
          items[i].ok = true;
          return true;
        } catch (const std::exception& e) {
          items[i].error = e.what();
          items[i].ok = false;
          return false;
        }
      },
      control,
      [&](std::size_t completed, std::size_t failed) {
        if (onProgress) {
          BatchProgress p;
          p.total = total;
          p.completed = completed;
          p.failed = failed;
          onProgress(p);
        }
      },
      threads);

  return items;
}

}  // namespace djcore
