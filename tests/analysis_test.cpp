#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <string>

#include "djcore/analysis/AnalysisPipeline.h"
#include "djcore/analysis/Flags.h"
#include "djcore/audio/AudioEncoder.h"
#include "djcore/audio/PcmBuffer.h"
#include "djcore/model/TargetProfile.h"

using namespace djcore;

namespace {

constexpr double kPi = 3.14159265358979323846;

// Analyze an in-memory buffer by writing it to a temp float WAV and running the
// pipeline (exercises the full decode->analyze path).
AnalysisResult analyzeBuffer(const PcmBuffer& buf, const char* name) {
  const std::string path =
      (std::filesystem::temp_directory_path() / name).string();
  encodeToFile(path, buf, EncodeSpec{"wav", buf.sampleRate(), 32, true});
  AnalysisPipeline pipeline;
  const AnalysisResult r = pipeline.analyzeFile(path);
  std::filesystem::remove(path);
  return r;
}

PcmBuffer stereo(int rate, std::size_t frames) { return PcmBuffer(2, rate, frames); }

}  // namespace

TEST(Analysis, LeadingSilenceDetectedWithinTolerance) {
  const int rate = 44100;
  const std::size_t lead = rate / 2;  // 500 ms of silence
  PcmBuffer buf = stereo(rate, rate * 2);
  for (std::size_t i = lead; i < buf.frames(); ++i) {
    const double t = static_cast<double>(i) / rate;
    const float v = static_cast<float>(0.5 * std::sin(2.0 * kPi * 1000.0 * t));
    buf.channel(0)[i] = v;
    buf.channel(1)[i] = v;
  }
  const AnalysisResult r = analyzeBuffer(buf, "djcore_sil.wav");
  EXPECT_NEAR(static_cast<double>(r.leadingSilenceMs), 500.0, 10.0);
}

TEST(Analysis, OutOfPhaseSignalFlaggedAsMonoRisk) {
  const int rate = 44100;
  PcmBuffer buf = stereo(rate, rate);
  for (std::size_t i = 0; i < buf.frames(); ++i) {
    const double t = static_cast<double>(i) / rate;
    const float v = static_cast<float>(0.5 * std::sin(2.0 * kPi * 440.0 * t));
    buf.channel(0)[i] = v;
    buf.channel(1)[i] = -v;  // fully inverted => cancels on mono sum
  }
  const AnalysisResult r = analyzeBuffer(buf, "djcore_oop.wav");
  ASSERT_TRUE(r.phaseCorrelation.has_value());
  EXPECT_NEAR(*r.phaseCorrelation, -1.0, 0.01);
  ASSERT_TRUE(r.monoFolddownDeltaDb.has_value());
  EXPECT_LT(*r.monoFolddownDeltaDb, -40.0);  // major cancellation
}

TEST(Analysis, InPhaseSignalIsMonoSafeAndNarrow) {
  const int rate = 44100;
  PcmBuffer buf = stereo(rate, rate);
  for (std::size_t i = 0; i < buf.frames(); ++i) {
    const double t = static_cast<double>(i) / rate;
    const float v = static_cast<float>(0.5 * std::sin(2.0 * kPi * 440.0 * t));
    buf.channel(0)[i] = v;
    buf.channel(1)[i] = v;
  }
  const AnalysisResult r = analyzeBuffer(buf, "djcore_inph.wav");
  ASSERT_TRUE(r.phaseCorrelation.has_value());
  EXPECT_NEAR(*r.phaseCorrelation, 1.0, 0.01);
  ASSERT_TRUE(r.monoFolddownDeltaDb.has_value());
  EXPECT_NEAR(*r.monoFolddownDeltaDb, 0.0, 0.5);  // no loss
  ASSERT_TRUE(r.stereoWidth.has_value());
  EXPECT_NEAR(*r.stereoWidth, 0.0, 0.01);  // mono-centred
}

TEST(Analysis, ChannelImbalanceDetected) {
  const int rate = 44100;
  PcmBuffer buf = stereo(rate, rate);
  const double gainR = std::pow(10.0, 3.0 / 20.0);  // +3 dB on right
  for (std::size_t i = 0; i < buf.frames(); ++i) {
    const double t = static_cast<double>(i) / rate;
    const float v = static_cast<float>(0.3 * std::sin(2.0 * kPi * 440.0 * t));
    buf.channel(0)[i] = v;
    buf.channel(1)[i] = static_cast<float>(v * gainR);
  }
  const AnalysisResult r = analyzeBuffer(buf, "djcore_bal.wav");
  ASSERT_TRUE(r.lrBalanceDb.has_value());
  EXPECT_NEAR(*r.lrBalanceDb, 3.0, 0.2);
}

TEST(Analysis, LoudnessTracksGainAndDrIsGainInvariant) {
  const int rate = 48000;
  const auto makeTone = [&](double amp) {
    PcmBuffer buf = stereo(rate, rate * 3);  // 3 s
    for (std::size_t i = 0; i < buf.frames(); ++i) {
      const double t = static_cast<double>(i) / rate;
      const float v = static_cast<float>(amp * std::sin(2.0 * kPi * 1000.0 * t));
      buf.channel(0)[i] = v;
      buf.channel(1)[i] = v;
    }
    return buf;
  };

  const AnalysisResult loud = analyzeBuffer(makeTone(0.5), "djcore_l1.wav");
  const AnalysisResult quiet = analyzeBuffer(makeTone(0.25), "djcore_l2.wav");

  // Halving amplitude lowers loudness by ~6 dB (gain-only).
  EXPECT_NEAR(loud.integratedLufs - quiet.integratedLufs, 6.0, 0.5);
  // Crest factor of a sine is constant regardless of gain (~sqrt(2)).
  EXPECT_NEAR(loud.crestFactor, quiet.crestFactor, 0.05);
  EXPECT_NEAR(loud.crestFactor, std::sqrt(2.0), 0.05);
}

TEST(Flags, ThreeStateClassification) {
  TargetProfile p;
  p.loudnessTargetLufs = -14.0;
  p.monoSafetyThreshold = 0.2;
  p.widthTolerance = {0.5, 0.25};

  AnalysisResult r;
  r.integratedLufs = -14.3;  // within ±1 LU
  EXPECT_EQ(loudnessFlag(r, p), FlagState::Ok);
  r.integratedLufs = -15.5;  // within 2x band (1<|Δ|<=2)
  EXPECT_EQ(loudnessFlag(r, p), FlagState::Review);
  r.integratedLufs = -20.0;  // far off
  EXPECT_EQ(loudnessFlag(r, p), FlagState::OutsideTarget);

  r.phaseCorrelation = 0.5;  // >= threshold
  EXPECT_EQ(monoFlag(r, p), FlagState::Ok);
  r.phaseCorrelation = -0.5;  // well below threshold
  EXPECT_EQ(monoFlag(r, p), FlagState::OutsideTarget);

  // Unset stereo metrics (mono track) read as OK, never a false alarm.
  AnalysisResult mono;
  EXPECT_EQ(monoFlag(mono, p), FlagState::Ok);
  EXPECT_EQ(widthFlag(mono, p), FlagState::Ok);
}
