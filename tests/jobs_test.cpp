#include <atomic>
#include <filesystem>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "djcore/audio/PcmBuffer.h"
#include "djcore/audio/WavIo.h"
#include "djcore/jobs/AnalysisBatch.h"
#include "djcore/jobs/JobControl.h"
#include "djcore/jobs/ParallelFor.h"

namespace fs = std::filesystem;
using namespace djcore;

TEST(Jobs, ParallelForRunsEveryItem) {
  constexpr std::size_t kCount = 200;
  std::atomic<long long> sum{0};
  JobControl control;
  parallelFor(kCount,
              [&](std::size_t i) {
                sum.fetch_add(static_cast<long long>(i));
                return true;
              },
              control);
  EXPECT_EQ(sum.load(), static_cast<long long>(kCount * (kCount - 1) / 2));
}

TEST(Jobs, CancelledBeforeStartRunsNothing) {
  std::atomic<int> ran{0};
  JobControl control;
  control.cancel();
  parallelFor(100, [&](std::size_t) { ran.fetch_add(1); return true; }, control);
  EXPECT_EQ(ran.load(), 0);
}

TEST(Jobs, JobControlPauseResumeState) {
  JobControl control;
  EXPECT_FALSE(control.isPaused());
  control.pause();
  EXPECT_TRUE(control.isPaused());
  control.resume();
  EXPECT_FALSE(control.isPaused());
  control.cancel();
  EXPECT_TRUE(control.isCancelled());
}

TEST(Jobs, AnalysisBatchAnalyzesAllFiles) {
  const auto dir = fs::temp_directory_path() / "djbatch";
  fs::create_directories(dir);
  std::vector<std::string> paths;
  for (int k = 0; k < 3; ++k) {
    PcmBuffer b(2, 44100, 4410);
    for (std::size_t i = 0; i < b.frames(); ++i) {
      b.channel(0)[i] = 0.3f;
      b.channel(1)[i] = 0.3f;
    }
    const auto p = (dir / ("t" + std::to_string(k) + ".wav")).string();
    writeWav(p, b, 16, false);
    paths.push_back(p);
  }

  JobControl control;
  AnalysisBatch batch;
  auto items = batch.run(paths, control);
  ASSERT_EQ(items.size(), 3u);
  for (const auto& it : items) {
    EXPECT_TRUE(it.ok);
    ASSERT_TRUE(it.result.has_value());
  }

  std::error_code ec;
  fs::remove_all(dir, ec);
}
