#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

#include "djcore/jobs/JobControl.h"

namespace djcore {

// Progress callback: (completed, failed). Invoked serially (under a lock) so the
// callee need not be thread-safe.
using ProgressCallback = std::function<void(std::size_t completed, std::size_t failed)>;

// Runs `worker(index)` for index in [0, count) across a pool sized to the
// hardware (NFR-PERF-2), honoring the JobControl's pause/cancel. `worker`
// returns true on success, false on failure; it should not throw (thrown
// exceptions are caught and counted as failures). Items are independent and
// dispatched dynamically so faster items don't wait on slower ones.
template <typename Fn>
void parallelFor(std::size_t count, Fn worker, JobControl& control,
                 const ProgressCallback& onProgress = {}, unsigned threads = 0) {
  if (threads == 0) {
    threads = std::max(1u, std::thread::hardware_concurrency());
  }
  threads = static_cast<unsigned>(std::min<std::size_t>(threads, std::max<std::size_t>(1, count)));

  std::atomic<std::size_t> next{0};
  std::atomic<std::size_t> completed{0};
  std::atomic<std::size_t> failed{0};
  std::mutex progressMutex;

  auto runner = [&]() {
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
      const std::size_t done = completed.fetch_add(1) + 1;
      const std::size_t fail = ok ? failed.load() : (failed.fetch_add(1) + 1);
      if (onProgress) {
        std::lock_guard<std::mutex> lock(progressMutex);
        onProgress(done, fail);
      }
    }
  };

  std::vector<std::thread> pool;
  pool.reserve(threads);
  for (unsigned t = 0; t < threads; ++t) pool.emplace_back(runner);
  for (auto& th : pool) th.join();
}

}  // namespace djcore
