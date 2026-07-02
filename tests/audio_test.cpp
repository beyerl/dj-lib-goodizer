#include <cmath>
#include <filesystem>

#include <gtest/gtest.h>

#include "djcore/audio/AudioDecoder.h"
#include "djcore/audio/PcmBuffer.h"
#include "djcore/audio/WavIo.h"

namespace fs = std::filesystem;
using namespace djcore;

namespace {

constexpr double kPi = 3.14159265358979323846;

fs::path tmpFile(const std::string& name) {
  return fs::temp_directory_path() / ("djtest_" + name);
}

PcmBuffer makeSine(int ch, int sr, std::size_t frames, double freq, float amp) {
  PcmBuffer b(ch, sr, frames);
  for (std::size_t i = 0; i < frames; ++i) {
    float v = amp * static_cast<float>(std::sin(2.0 * kPi * freq *
                                                 static_cast<double>(i) / sr));
    for (int c = 0; c < ch; ++c) b.channel(c)[i] = v;
  }
  return b;
}

}  // namespace

TEST(WavIo, Float32RoundTripIsExact) {
  PcmBuffer b(2, 44100, 5);
  const float L[5] = {0.0f, 0.5f, -0.5f, 0.999f, -0.999f};
  const float R[5] = {0.1f, -0.2f, 0.3f, -0.4f, 0.25f};
  for (int i = 0; i < 5; ++i) {
    b.channel(0)[i] = L[i];
    b.channel(1)[i] = R[i];
  }
  const auto p = tmpFile("f32.wav");
  writeWav(p.string(), b, 32, /*floatFormat=*/true);

  FormatInfo info;
  PcmBuffer r = readWav(p.string(), &info);
  ASSERT_EQ(r.channels(), 2);
  ASSERT_EQ(r.frames(), 5u);
  EXPECT_EQ(info.sampleRate, 44100);
  EXPECT_EQ(info.codec, "pcm_f32le");
  for (int i = 0; i < 5; ++i) {
    EXPECT_FLOAT_EQ(r.channel(0)[i], L[i]);
    EXPECT_FLOAT_EQ(r.channel(1)[i], R[i]);
  }
  fs::remove(p);
}

TEST(WavIo, Pcm16RoundTripWithinQuantization) {
  PcmBuffer b = makeSine(1, 48000, 480, 1000.0, 0.5f);
  const auto p = tmpFile("s16.wav");
  writeWav(p.string(), b, 16, false);

  FormatInfo info;
  PcmBuffer r = readWav(p.string(), &info);
  ASSERT_EQ(r.frames(), 480u);
  EXPECT_EQ(info.bitDepth, 16);
  for (std::size_t i = 0; i < 480; ++i) {
    EXPECT_NEAR(r.channel(0)[i], b.channel(0)[i], 1.0f / 32000.0f);
  }
  fs::remove(p);
}

TEST(WavIo, Pcm24RoundTripWithinQuantization) {
  PcmBuffer b = makeSine(2, 44100, 300, 440.0, 0.7f);
  const auto p = tmpFile("s24.wav");
  writeWav(p.string(), b, 24, false);

  FormatInfo info;
  PcmBuffer r = readWav(p.string(), &info);
  ASSERT_EQ(r.frames(), 300u);
  EXPECT_EQ(info.bitDepth, 24);
  for (std::size_t i = 0; i < 300; ++i) {
    EXPECT_NEAR(r.channel(0)[i], b.channel(0)[i], 1.0f / 8000000.0f);
    EXPECT_NEAR(r.channel(1)[i], b.channel(1)[i], 1.0f / 8000000.0f);
  }
  fs::remove(p);
}

TEST(WavIo, ProbeMatchesFullRead) {
  PcmBuffer b = makeSine(2, 44100, 1000, 440.0, 0.3f);
  const auto p = tmpFile("probe.wav");
  writeWav(p.string(), b, 24, false);

  FormatInfo full;
  readWav(p.string(), &full);
  FormatInfo probed = probeWav(p.string());
  EXPECT_EQ(probed.sampleRate, full.sampleRate);
  EXPECT_EQ(probed.channels, full.channels);
  EXPECT_EQ(probed.bitDepth, full.bitDepth);
  EXPECT_EQ(probed.durationMs, full.durationMs);
  fs::remove(p);
}

TEST(AudioDecoder, OpensAndDecodesWav) {
  PcmBuffer b = makeSine(1, 44100, 100, 440.0, 0.2f);
  const auto p = tmpFile("dec.wav");
  writeWav(p.string(), b, 16, false);

  EXPECT_TRUE(canDecode(p.string()));
  auto d = openDecoder(p.string());
  EXPECT_EQ(d->format().sampleRate, 44100);
  EXPECT_EQ(d->format().channels, 1);
  PcmBuffer buf = d->readAll();
  EXPECT_EQ(buf.frames(), 100u);
  fs::remove(p);
}
