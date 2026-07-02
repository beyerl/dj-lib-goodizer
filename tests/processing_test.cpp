#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include <gtest/gtest.h>

#include "djcore/audio/Dither.h"
#include "djcore/audio/PcmBuffer.h"
#include "djcore/audio/Resampler.h"
#include "djcore/audio/WavIo.h"
#include "djcore/processing/Operations.h"
#include "djcore/processing/ProcessingEngine.h"
#include "djcore/profile/DefaultProfiles.h"

namespace fs = std::filesystem;
using namespace djcore;

namespace {

constexpr double kPi = 3.14159265358979323846;

PcmBuffer makeSine(int ch, int sr, std::size_t frames, double freq, float amp) {
  PcmBuffer b(ch, sr, frames);
  for (std::size_t i = 0; i < frames; ++i) {
    float v = amp * static_cast<float>(std::sin(2.0 * kPi * freq *
                                                 static_cast<double>(i) / sr));
    for (int c = 0; c < ch; ++c) b.channel(c)[i] = v;
  }
  return b;
}

double rms(const PcmBuffer& b, int c) {
  double s = 0.0;
  for (std::size_t i = 0; i < b.frames(); ++i) s += b.channel(c)[i] * b.channel(c)[i];
  return b.frames() ? std::sqrt(s / b.frames()) : 0.0;
}

double peak(const PcmBuffer& b, int c) {
  double p = 0.0;
  for (std::size_t i = 0; i < b.frames(); ++i) p = std::max(p, std::fabs((double)b.channel(c)[i]));
  return p;
}

fs::path tmp(const std::string& n) { return fs::temp_directory_path() / ("djp_" + n); }

std::vector<unsigned char> readBytes(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

}  // namespace

TEST(Resampler, UpsamplePreservesLengthAndRms) {
  PcmBuffer b = makeSine(1, 44100, 44100, 1000.0, 0.5f);  // 1 s
  PcmBuffer r = resample(b, 48000);
  EXPECT_EQ(r.sampleRate(), 48000);
  EXPECT_NEAR(static_cast<double>(r.frames()), 48000.0, 2.0);
  EXPECT_NEAR(rms(r, 0), rms(b, 0), 0.02);
}

TEST(Resampler, DownsamplePreservesRms) {
  PcmBuffer b = makeSine(1, 48000, 48000, 1000.0, 0.5f);
  PcmBuffer r = resample(b, 44100);
  EXPECT_NEAR(static_cast<double>(r.frames()), 44100.0, 2.0);
  EXPECT_NEAR(rms(r, 0), rms(b, 0), 0.02);
}

TEST(Resampler, SameRateReturnsCopy) {
  PcmBuffer b = makeSine(2, 44100, 128, 440.0, 0.3f);
  PcmBuffer r = resample(b, 44100);
  EXPECT_EQ(r.frames(), 128u);
  EXPECT_EQ(r.sampleRate(), 44100);
}

TEST(Gain, ScalesAndPreservesCrest) {
  PcmBuffer b = makeSine(1, 44100, 4410, 440.0, 0.5f);
  const double crestBefore = peak(b, 0) / rms(b, 0);
  GainOperation(0.5).apply(b);
  EXPECT_NEAR(peak(b, 0), 0.25, 1e-3);
  EXPECT_NEAR(peak(b, 0) / rms(b, 0), crestBefore, 1e-4);  // crest unchanged
}

TEST(Dither, DeterministicAndZeroMean) {
  TpdfDither a(123), b(123);
  double sum = 0.0;
  for (int i = 0; i < 100000; ++i) {
    double va = a.next();
    double vb = b.next();
    EXPECT_DOUBLE_EQ(va, vb);  // same seed -> same sequence
    sum += va;
  }
  EXPECT_NEAR(sum / 100000.0, 0.0, 0.02);  // zero-mean
}

TEST(ProcessingEngine, ConformSkipCopiesWhenAlreadyConforming) {
  PcmBuffer b = makeSine(2, 44100, 4410, 440.0, 0.4f);
  const auto src = tmp("conform_src.wav");
  writeWav(src.string(), b, 16, false);

  Track t;
  t.sourcePath = src.string();
  t.format = probeWav(src.string());

  auto profile = *defaultProfileByName("Archival / Lossless");
  ProcessingEngine engine(profile);
  const auto out = tmp("conform_out.wav");
  ProcessingChange ch = engine.process(t, AnalysisResult{}, out.string());

  EXPECT_TRUE(ch.passthrough);
  EXPECT_EQ(readBytes(src), readBytes(out));  // byte-identical lossless copy
  fs::remove(src);
  fs::remove(out);
}

TEST(ProcessingEngine, StandardizesToClubProfile) {
  PcmBuffer b = makeSine(2, 48000, 48000, 1000.0, 0.5f);  // 48k/24 stereo
  const auto src = tmp("club_src.wav");
  writeWav(src.string(), b, 24, false);

  Track t;
  t.sourcePath = src.string();
  t.format = probeWav(src.string());

  auto profile = *defaultProfileByName("Club / USB");  // 44.1k / 16-bit
  ProcessingEngine engine(profile);
  const auto out = tmp("club_out.wav");
  ProcessingChange ch = engine.process(t, AnalysisResult{}, out.string());

  EXPECT_TRUE(ch.resampled);
  EXPECT_TRUE(ch.requantized);
  FormatInfo info;
  PcmBuffer r = readWav(out.string(), &info);
  EXPECT_EQ(info.sampleRate, 44100);
  EXPECT_EQ(info.bitDepth, 16);
  EXPECT_NEAR(static_cast<double>(r.frames()), 44100.0, 5.0);
  fs::remove(src);
  fs::remove(out);
}
