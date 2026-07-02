#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace djcore {

// Cooperative pause/resume/cancel token for batch workers. Workers poll it
// between items (never mid-item), so no partially-written output can result.
class JobControl {
 public:
  void pause();
  void resume();
  void cancel();

  bool isCancelled() const { return cancelled_.load(); }
  bool isPaused() const;

  // Blocks the caller while paused; returns immediately if cancelled.
  void waitWhilePaused() const;

 private:
  mutable std::mutex mutex_;
  mutable std::condition_variable cv_;
  std::atomic<bool> cancelled_{false};
  bool paused_ = false;
};

}  // namespace djcore
