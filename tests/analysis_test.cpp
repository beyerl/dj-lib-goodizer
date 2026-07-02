#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "djcore/analysis/analyzers/LoudnessDynamicsAnalyzer.h"
#include "djcore/analysis/analyzers/PhaseMonoAnalyzer.h"
#include "djcore/analysis/analyzers/SilenceDetector.h"
#include "djcore/analysis/analyzers/StereoWidthAnalyzer.h"
#include "djcore/audio/PcmBuffer.h"

using namespace djcore;

namespace {

constexpr double kPi = 3.14159265358979323846;

PcmBuffer makeSine(int ch, int sr, std::size_t frames, double freq, float amp,
                   float rightScale = 1.0f) {
  PcmBuffer b(ch, sr, frames);
  for (std::size_t i = 0; i < frames; ++i) {
    float v = amp * static_cast<float>(std::sin(2.0 * kPi * freq *
                                                 static_cast<double>(i) / sr));
    for (int c = 0; c < ch; ++c) b.channel(c)[i] = (c == 1 ? rightScale : 1.0f) * v;
  }
  return b;
}

AnalysisResult runAnalyzer(IAnalyzer& a, const PcmBuffer& buf) {
  FormatInfo f;
  f.sampleRate = buf.sampleRate();
  f.channels = buf.channels();
  f.bitDepth = 32;
  a.begin(f);
  std::vector<const float*> planes(buf.channels());
  for (int c = 0; c < buf.channels(); ++c) planes[c] = buf.channel(c);
  PcmBlock blk;
  blk.planes = planes.data();
  blk.channels = buf.channels();
  blk.sampleRate = buf.sampleRate();
  blk.frames = buf.frames();
  a.processBlock(blk);
  AnalysisResult r;
  a.finalize(r);
  return r;
}

}  // namespace

TEST(Silence, LeadingSilenceWithinTolerance) {
  // 500 ms of digital silence, then a 1 kHz tone.
  PcmBuffer b(2, 44100, 44100);  // 1 s
  const std::size_t lead = 22050;  // 500 ms
  for (std::size_t i = lead; i < b.frames(); ++i) {
    float v = 0.5f * static_cast<float>(std::sin(2.0 * kPi * 1000.0 * i / 44100.0));
    b.channel(0)[i] = v;
    b.channel(1)[i] = v;
  }
  SilenceDetector det;
  AnalysisResult r = runAnalyzer(det, b);
  EXPECT_NEAR(static_cast<double>(r.leadingSilenceMs), 500.0, 15.0);
}

TEST(PhaseMono, InPhaseIsMonoSafe) {
  PcmBuffer b = makeSine(2, 48000, 4800, 440.0, 0.5f);  // L==R
  PhaseMonoAnalyzer a;
  AnalysisResult r = runAnalyzer(a, b);
  ASSERT_TRUE(r.phaseCorrelation.has_value());
  EXPECT_NEAR(*r.phaseCorrelation, 1.0, 0.01);
  ASSERT_TRUE(r.monoFolddownDeltaDb.has_value());
  EXPECT_NEAR(*r.monoFolddownDeltaDb, 0.0, 0.5);
}

TEST(PhaseMono, OutOfPhaseCancels) {
  PcmBuffer b = makeSine(2, 48000, 4800, 440.0, 0.5f, /*rightScale=*/-1.0f);  // R=-L
  PhaseMonoAnalyzer a;
  AnalysisResult r = runAnalyzer(a, b);
  ASSERT_TRUE(r.phaseCorrelation.has_value());
  EXPECT_NEAR(*r.phaseCorrelation, -1.0, 0.01);
  ASSERT_TRUE(r.monoFolddownDeltaDb.has_value());
  EXPECT_LT(*r.monoFolddownDeltaDb, -40.0);
}

TEST(StereoWidth, DualMonoIsNarrow) {
  PcmBuffer b = makeSine(2, 48000, 4800, 440.0, 0.5f);  // L==R
  StereoWidthAnalyzer a;
  AnalysisResult r = runAnalyzer(a, b);
  ASSERT_TRUE(r.stereoWidth.has_value());
  EXPECT_NEAR(*r.stereoWidth, 0.0, 0.01);
}

TEST(StereoWidth, BalanceDetectsPlus3dBRight) {
  const float rightScale = static_cast<float>(std::pow(10.0, 3.0 / 20.0));  // +3 dB
  PcmBuffer b = makeSine(2, 48000, 4800, 440.0, 0.5f, rightScale);
  StereoWidthAnalyzer a;
  AnalysisResult r = runAnalyzer(a, b);
  ASSERT_TRUE(r.lrBalanceDb.has_value());
  EXPECT_NEAR(*r.lrBalanceDb, 3.0, 0.2);
}

TEST(Loudness, TracksGainBy6LuWhenHalved) {
  PcmBuffer loud = makeSine(2, 48000, 48000, 1000.0, 0.5f);
  PcmBuffer quiet = makeSine(2, 48000, 48000, 1000.0, 0.25f);
  LoudnessDynamicsAnalyzer a1, a2;
  const double lLoud = runAnalyzer(a1, loud).integratedLufs;
  const double lQuiet = runAnalyzer(a2, quiet).integratedLufs;
  EXPECT_NEAR(lLoud - lQuiet, 6.02, 0.8);
}

TEST(Loudness, CrestAndDrAreGainInvariant) {
  PcmBuffer loud = makeSine(1, 48000, 48000, 1000.0, 0.5f);
  PcmBuffer quiet = makeSine(1, 48000, 48000, 1000.0, 0.25f);
  LoudnessDynamicsAnalyzer a1, a2;
  AnalysisResult rl = runAnalyzer(a1, loud);
  AnalysisResult rq = runAnalyzer(a2, quiet);
  EXPECT_NEAR(rl.crestFactor, rq.crestFactor, 1e-3);
  EXPECT_NEAR(rl.crestFactor, std::sqrt(2.0), 0.05);  // sine crest = sqrt(2)
  EXPECT_NEAR(rl.drValue, rq.drValue, 0.05);
}

TEST(Loudness, TruePeakBeatsSamplePeakOnIntersamplePeak) {
  // fs/4 tone phased so samples sit at +/-0.707 (sample peak -3 dBFS, true ~0).
  PcmBuffer b(1, 48000, 4000);
  for (std::size_t i = 0; i < b.frames(); ++i) {
    b.channel(0)[i] =
        static_cast<float>(std::sin(kPi * i / 2.0 + kPi / 4.0));  // amplitude 1.0
  }
  LoudnessDynamicsAnalyzer a;
  AnalysisResult r = runAnalyzer(a, b);
  EXPECT_GT(r.truePeakDbtp, -2.0);   // clearly above the -3.01 sample peak
  EXPECT_LE(r.truePeakDbtp, 0.5);
}
