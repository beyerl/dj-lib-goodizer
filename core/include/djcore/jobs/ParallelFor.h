#pragma once

#include <cstddef>
#include <functional>

#include "djcore/jobs/JobControl.h"

namespace djcore {

using ProgressCallback =
    std::function<void(std::size_t completed, std::size_t failed)>;

// Runs worker(i) for i in [0, count) across a thread pool sized to `threads`
// (0 = hardware concurrency). worker returns true on success; exceptions are
// caught and counted as failures. Cancellation/pause are checked between items.
void parallelFor(std::size_t count,
                 const std::function<bool(std::size_t)>& worker,
                 JobControl& control, const ProgressCallback& onProgress = {},
                 unsigned threads = 0);

}  // namespace djcore
