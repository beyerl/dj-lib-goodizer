#pragma once

#include <condition_variable>
#include <mutex>

namespace djcore {

// Cooperative pause/resume/cancel handle shared between the UI and worker
// threads (plan §5). Workers poll isCancelled() between items and block in
// waitWhilePaused() when paused; neither interrupts work mid-item, so no
// partially-processed output is ever left inconsistent (NFR-PERF-3).
class JobControl {
 public:
  void pause() {
    std::lock_guard<std::mutex> lock(m_);
    paused_ = true;
  }

  void resume() {
    {
      std::lock_guard<std::mutex> lock(m_);
      paused_ = false;
    }
    cv_.notify_all();
  }

  void cancel() {
    {
      std::lock_guard<std::mutex> lock(m_);
      cancelled_ = true;
      paused_ = false;  // unblock any paused workers so they can exit
    }
    cv_.notify_all();
  }

  bool isCancelled() const {
    std::lock_guard<std::mutex> lock(m_);
    return cancelled_;
  }

  bool isPaused() const {
    std::lock_guard<std::mutex> lock(m_);
    return paused_;
  }

  // Blocks while paused; returns immediately if cancelled.
  void waitWhilePaused() const {
    std::unique_lock<std::mutex> lock(m_);
    cv_.wait(lock, [this] { return !paused_ || cancelled_; });
  }

 private:
  mutable std::mutex m_;
  mutable std::condition_variable cv_;
  bool paused_ = false;
  bool cancelled_ = false;
};

}  // namespace djcore
