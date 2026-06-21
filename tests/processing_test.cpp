#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>

#include "djcore/analysis/AnalysisPipeline.h"
#include "djcore/audio/AudioDecoder.h"
#include "djcore/audio/AudioEncoder.h"
#include "djcore/audio/PcmBuffer.h"
#include "djcore/db/AuditLogRepository.h"
#include "djcore/db/Database.h"
#include "djcore/db/Schema.h"
#include "djcore/db/TrackRepository.h"
#include "djcore/model/TargetProfile.h"
#include "djcore/processing/Operations.h"
#include "djcore/processing/ProcessingEngine.h"

using namespace djcore;

namespace {
constexpr double kPi = 3.14159265358979323846;

std::string tmp(const char* n) {
  return (std::filesystem::temp_directory_path() / n).string();
}

// 48 kHz / 24-bit stereo: 300 ms leading silence then a 1 kHz tone.
std::string makeSource() {
  const int rate = 48000;
  PcmBuffer buf(2, rate, rate * 2);
  const std::size_t lead = static_cast<std::size_t>(rate * 0.3);
  for (std::size_t i = lead; i < buf.frames(); ++i) {
    const double t = static_cast<double>(i) / rate;
    const float v = static_cast<float>(0.6 * std::sin(2.0 * kPi * 1000.0 * t));
    buf.channel(0)[i] = v;
    buf.channel(1)[i] = v;
  }
  const std::string p = tmp("djcore_src.wav");
  encodeToFile(p, buf, EncodeSpec{"wav", rate, 24, false});
  return p;
}

TargetProfile wavTarget() {
  TargetProfile p;
  p.name = "Test WAV";
  p.container = "wav";  // dependency-free encoder supports WAV
  p.sampleRate = 44100;
  p.bitDepth = 16;
  p.loudnessTargetLufs = -18.0;
  p.silence.leadInMs = 10;  // keep 10 ms lead-in
  return p;
}

}  // namespace

TEST(Trim, ZeroCrossingTrimRemovesFrames) {
  PcmBuffer buf(1, 1000, 1000);
  for (std::size_t i = 0; i < buf.frames(); ++i) {
    buf.channel(0)[i] = static_cast<float>(std::sin(2.0 * kPi * 5.0 * i / 1000.0));
  }
  const PcmBuffer out = trimSilence(buf, 100, 100);
  EXPECT_LE(out.frames(), 900u);
  EXPECT_GE(out.frames(), 780u);  // near 800, snapped to zero crossings
}

TEST(Processing, PlanReportsConformanceChanges) {
  Track t;
  t.format.container = "wav";
  t.format.sampleRate = 48000;
  t.format.bitDepth = 24;
  AnalysisResult a;
  a.integratedLufs = -12.0;
  a.truePeakDbtp = -3.0;
  a.leadingSilenceMs = 300;

  ProcessingEngine engine(wavTarget());
  const ProcessingChange ch = engine.plan(t, a);
  EXPECT_TRUE(ch.resampled);
  EXPECT_EQ(ch.toRate, 44100);
  EXPECT_TRUE(ch.requantized);
  EXPECT_EQ(ch.toBits, 16);
  EXPECT_TRUE(ch.trimmed);
  EXPECT_EQ(ch.trimLeadMs, 290);  // 300 - 10 ms lead-in retained
  EXPECT_TRUE(ch.gainApplied);
  EXPECT_NEAR(ch.gainDb, -6.0, 0.01);  // -18 target from -12 measured
  EXPECT_FALSE(ch.passthrough);
}

TEST(Processing, EndToEndConformsAndPreservesDynamics) {
  const std::string src = makeSource();

  AnalysisPipeline pipeline;
  const AnalysisResult before = pipeline.analyzeFile(src);

  Track t;
  t.sourcePath = src;
  t.format.container = "wav";
  t.format.sampleRate = 48000;
  t.format.bitDepth = 24;

  ProcessingEngine engine(wavTarget());
  const std::string out = tmp("djcore_out.wav");
  const ProcessingChange ch = engine.process(t, before, out);
  ASSERT_TRUE(std::filesystem::exists(out));

  auto dec = openDecoder(out);
  EXPECT_EQ(dec->format().sampleRate, 44100);  // resampled
  EXPECT_EQ(dec->format().bitDepth, 16);       // requantized

  const AnalysisResult after = pipeline.analyzeFile(out);
  // Leading silence trimmed down toward the retained 10 ms lead-in.
  EXPECT_LT(after.leadingSilenceMs, 60);
  // Loudness moved toward the -18 LUFS target.
  EXPECT_NEAR(after.integratedLufs, -18.0, 1.5);

  std::filesystem::remove(src);
  std::filesystem::remove(out);
}

TEST(Gain, PreservesCrestFactorExactly) {
  // AC 3.3.3: gain-only normalization scales peak and RMS together, so crest
  // factor (and dynamic range) is unchanged.
  PcmBuffer buf(1, 44100, 4096);
  for (std::size_t i = 0; i < buf.frames(); ++i) {
    buf.channel(0)[i] =
        static_cast<float>(0.7 * std::sin(2.0 * kPi * 1000.0 * i / 44100.0));
  }
  const auto crest = [](const PcmBuffer& b) {
    double peak = 0.0, ss = 0.0;
    for (std::size_t i = 0; i < b.frames(); ++i) {
      const double x = b.channel(0)[i];
      peak = std::max(peak, std::abs(x));
      ss += x * x;
    }
    return peak / std::sqrt(ss / b.frames());
  };
  const double before = crest(buf);
  GainOperation(std::pow(10.0, -6.0 / 20.0)).apply(buf);  // -6 dB
  EXPECT_NEAR(crest(buf), before, 1e-4);
}

TEST(Audit, LogEntryPersists) {
  Database db(":memory:");
  migrate(db);
  TrackRepository tracks(db);
  AuditLogRepository audit(db);

  Track tr;
  tr.sourcePath = "/music/x.wav";
  const std::int64_t trackId = tracks.insert(tr);

  AuditLogEntry e;
  e.trackId = trackId;
  e.operation = OperationType::GainNormalize;
  e.paramsJson = "{\"gainDb\":-6.0}";
  e.beforeMetricsJson = "{\"lufs\":-12.0}";
  e.afterMetricsJson = "{\"lufs\":-18.0}";
  e.timestampUnix = 1700000000;
  audit.insert(e);

  EXPECT_EQ(audit.count(), 1);
  const auto rows = audit.byTrack(trackId);
  ASSERT_EQ(rows.size(), 1u);
  EXPECT_EQ(rows[0].operation, OperationType::GainNormalize);
  EXPECT_EQ(rows[0].paramsJson, "{\"gainDb\":-6.0}");
}
