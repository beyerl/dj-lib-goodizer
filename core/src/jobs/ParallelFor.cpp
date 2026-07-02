#include "djcore/jobs/ParallelFor.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace djcore {

void parallelFor(std::size_t count,
                 const std::function<bool(std::size_t)>& worker,
                 JobControl& control, const ProgressCallback& onProgress,
                 unsigned threads) {
  if (count == 0) return;

  unsigned n = threads != 0 ? threads : std::thread::hardware_concurrency();
  if (n == 0) n = 1;
  if (n > count) n = static_cast<unsigned>(count);

  std::atomic<std::size_t> next{0};
  std::atomic<std::size_t> completed{0};
  std::atomic<std::size_t> failed{0};
  std::mutex progressMutex;

  auto run = [&]() {
    for (;;) {
      if (control.isCancelled()) return;
      control.waitWhilePaused();
      if (control.isCancelled()) return;

      const std::size_t i = next.fetch_add(1);
      if (i >= count) return;

      bool ok = false;
      try {
        ok = worker(i);
      } catch (...) {
        ok = false;
      }
      if (ok)
        completed.fetch_add(1);
      else
        failed.fetch_add(1);

      if (onProgress) {
        std::lock_guard<std::mutex> lock(progressMutex);
        onProgress(completed.load(), failed.load());
      }
    }
  };

  std::vector<std::thread> pool;
  pool.reserve(n);
  for (unsigned t = 0; t < n; ++t) pool.emplace_back(run);
  for (auto& th : pool) th.join();
}

}  // namespace djcore
