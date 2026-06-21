#include <gtest/gtest.h>

#include <string>

#include "djcore/Version.h"
#include "djcore/audio/PcmBuffer.h"
#include "djcore/model/AnalysisResult.h"
#include "djcore/profile/DefaultProfiles.h"

using namespace djcore;

TEST(Smoke, CoreVersionIsNonEmpty) {
  EXPECT_FALSE(std::string(coreVersionString()).empty());
}

TEST(Smoke, AnalyzerVersionIsPositive) {
  EXPECT_GT(kAnalyzerVersion, 0);
}

TEST(Smoke, ShipsAtLeastThreeDefaultProfiles) {
  const auto profiles = defaultProfiles();
  ASSERT_GE(profiles.size(), 3u);
  EXPECT_EQ(profiles.front().name, "Club / USB");  // recommended default first
}

TEST(Smoke, PcmBufferAllocatesPlanarChannels) {
  PcmBuffer buf(2, 44100, 1024);
  EXPECT_EQ(buf.channels(), 2);
  EXPECT_EQ(buf.sampleRate(), 44100);
  EXPECT_EQ(buf.frames(), 1024u);
  // Planar storage is zero-initialized and independently addressable.
  EXPECT_EQ(buf.channel(0)[0], 0.0f);
  buf.channel(1)[10] = 0.5f;
  EXPECT_EQ(buf.channel(1)[10], 0.5f);
  EXPECT_EQ(buf.channel(0)[10], 0.0f);
}

TEST(Smoke, AnalysisResultStereoMetricsDefaultUnset) {
  AnalysisResult r;
  EXPECT_FALSE(r.phaseCorrelation.has_value());
  EXPECT_FALSE(r.stereoWidth.has_value());
}
