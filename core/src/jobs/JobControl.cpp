#include "djcore/jobs/JobControl.h"

namespace djcore {

void JobControl::pause() {
  std::lock_guard<std::mutex> lock(mutex_);
  paused_ = true;
}

void JobControl::resume() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    paused_ = false;
  }
  cv_.notify_all();
}

void JobControl::cancel() {
  cancelled_.store(true);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    paused_ = false;
  }
  cv_.notify_all();
}

bool JobControl::isPaused() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return paused_;
}

void JobControl::waitWhilePaused() const {
  std::unique_lock<std::mutex> lock(mutex_);
  cv_.wait(lock, [&] { return !paused_ || cancelled_.load(); });
}

}  // namespace djcore
