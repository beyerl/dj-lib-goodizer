#pragma once

#include <cstdint>
#include <string>

namespace djcore {

// A tolerance band for a continuous metric (loudness, DR, width).
struct ToleranceBand {
  double target = 0.0;
  double plusMinus = 0.0;
};

// Silence-trim policy.
struct SilencePolicy {
  double thresholdDb = -60.0;  // level below which audio counts as silence
  int minDurationMs = 100;     // ignore silent runs shorter than this
  int leadInMs = 0;            // fixed lead-in to preserve when trimming
};

// A named, reusable standardization target. sampleRate/bitDepth of 0 mean
// "keep the source value" (lossless passthrough of that dimension).
struct TargetProfile {
  std::int64_t id = 0;
  std::string name;
  std::string container = "wav";
  int sampleRate = 0;
  int bitDepth = 0;

  SilencePolicy silence;
  double loudnessTargetLufs = 0.0;  // 0 = no loudness normalization
  ToleranceBand drTolerance;
  ToleranceBand widthTolerance;
  double monoSafetyThreshold = 0.2;
};

}  // namespace djcore
