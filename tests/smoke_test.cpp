#include <cstring>

#include <gtest/gtest.h>

#include "djcore/Version.h"
#include "djcore/audio/PcmBuffer.h"

TEST(Smoke, VersionIsNonEmpty) {
  EXPECT_GT(std::strlen(djcore::versionString()), 0u);
}

TEST(Smoke, PcmBufferConstruction) {
  djcore::PcmBuffer buf(2, 44100, 100);
  EXPECT_EQ(buf.channels(), 2);
  EXPECT_EQ(buf.sampleRate(), 44100);
  EXPECT_EQ(buf.frames(), 100u);
  EXPECT_FALSE(buf.empty());
}

TEST(Smoke, PcmBufferChannelAccessAndResize) {
  djcore::PcmBuffer buf(2, 48000, 8);
  ASSERT_NE(buf.channel(0), nullptr);
  ASSERT_NE(buf.channel(1), nullptr);

  // Freshly constructed planes are zero-filled.
  EXPECT_FLOAT_EQ(buf.channel(0)[0], 0.0f);

  buf.channel(0)[3] = 0.5f;
  buf.channel(1)[7] = -0.25f;
  EXPECT_FLOAT_EQ(buf.channel(0)[3], 0.5f);
  EXPECT_FLOAT_EQ(buf.channel(1)[7], -0.25f);

  // Growing preserves existing samples and zero-fills the rest.
  buf.resize(16);
  EXPECT_EQ(buf.frames(), 16u);
  EXPECT_FLOAT_EQ(buf.channel(0)[3], 0.5f);
  EXPECT_FLOAT_EQ(buf.channel(0)[15], 0.0f);
}

TEST(Smoke, PcmBufferDefaultIsEmpty) {
  djcore::PcmBuffer buf;
  EXPECT_TRUE(buf.empty());
  EXPECT_EQ(buf.channels(), 0);
  EXPECT_EQ(buf.frames(), 0u);
}
