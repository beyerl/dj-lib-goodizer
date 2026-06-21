#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <string>
#include <vector>

#include "djcore/audio/AudioEncoder.h"
#include "djcore/audio/PcmBuffer.h"
#include "djcore/jobs/AnalysisBatch.h"
#include "djcore/jobs/JobControl.h"
#include "djcore/jobs/ParallelFor.h"

using namespace djcore;

TEST(ParallelFor, ProcessesEveryItem) {
  JobControl control;
  std::atomic<std::int64_t> sum{0};
  parallelFor(
      100,
      [&](std::size_t i) {
        sum.fetch_add(static_cast<std::int64_t>(i));
        return true;
      },
      control);
  EXPECT_EQ(sum.load(), 4950);  // sum 0..99
}

TEST(ParallelFor, CountsFailuresAndReportsProgress) {
  JobControl control;
  std::atomic<std::size_t> lastCompleted{0};
  std::size_t finalFailed = 0;
  parallelFor(
      50, [&](std::size_t i) { return i % 2 == 0; },  // odd indices "fail"
      control,
      [&](std::size_t completed, std::size_t failed) {
        lastCompleted.store(completed);
        finalFailed = failed;
      });
  EXPECT_EQ(lastCompleted.load(), 50u);
  EXPECT_EQ(finalFailed, 25u);
}

TEST(ParallelFor, CancelledBeforeRunDoesNothing) {
  JobControl control;
  control.cancel();
  std::atomic<int> ran{0};
  parallelFor(
      1000,
      [&](std::size_t) {
        ran.fetch_add(1);
        return true;
      },
      control);
  EXPECT_EQ(ran.load(), 0);
}

TEST(AnalysisBatch, AnalyzesAllFiles) {
  const int rate = 44100;
  std::vector<std::string> paths;
  for (int f = 0; f < 3; ++f) {
    PcmBuffer buf(2, rate, rate / 2);
    for (std::size_t i = 0; i < buf.frames(); ++i) {
      const double t = static_cast<double>(i) / rate;
      const float v = static_cast<float>(0.4 * std::sin(2.0 * 3.14159265 * (440 + f * 110) * t));
      buf.channel(0)[i] = v;
      buf.channel(1)[i] = v;
    }
    const std::string p =
        (std::filesystem::temp_directory_path() / ("djcore_batch_" + std::to_string(f) + ".wav"))
            .string();
    encodeToFile(p, buf, EncodeSpec{"wav", rate, 16, false});
    paths.push_back(p);
  }

  JobControl control;
  AnalysisBatch batch;
  BatchProgress last;
  const auto items = batch.run(paths, control,
                               [&](const BatchProgress& bp) { last = bp; });

  ASSERT_EQ(items.size(), 3u);
  for (const auto& it : items) {
    EXPECT_TRUE(it.result.has_value()) << it.error;
    if (it.result) {
      ASSERT_TRUE(it.result->phaseCorrelation.has_value());
      EXPECT_NEAR(*it.result->phaseCorrelation, 1.0, 0.01);  // in-phase
    }
  }
  EXPECT_EQ(last.completed, 3u);
  EXPECT_EQ(last.failed, 0u);

  for (const auto& p : paths) std::filesystem::remove(p);
}
