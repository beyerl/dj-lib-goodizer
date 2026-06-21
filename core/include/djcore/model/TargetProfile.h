#pragma once

#include <cstdint>
#include <string>

namespace djcore {

// A symmetric tolerance band around a target value (e.g. DR value ± Y).
struct ToleranceBand {
  double target = 0.0;
  double plusMinus = 0.0;
};

// Silence detection / trim policy (FR-SIL-1/3).
struct SilencePolicy {
  double thresholdDb = -60.0;   // relative level treated as silence
  int minDurationMs = 100;      // min run length to qualify as silence
  int leadInMs = 0;             // retained lead-in after trim (0 = trim flush)
};

// A named, reusable standardization target (NFR-EXT-2, spec Table 11).
struct TargetProfile {
  std::int64_t id = 0;
  std::string name;

  // Feature Area A — format/sample-rate/bit-depth target.
  std::string container = "flac";
  int sampleRate = 44100;
  int bitDepth = 16;

  // Feature Area B — silence policy.
  SilencePolicy silence;

  // Feature Area C — loudness target + dynamic-range tolerance band.
  double loudnessTargetLufs = -14.0;
  ToleranceBand drTolerance{8.0, 2.0};

  // Feature Area D — minimum acceptable mono-safety (phase correlation).
  double monoSafetyThreshold = 0.2;

  // Feature Area E — stereo-width tolerance band.
  ToleranceBand widthTolerance{0.5, 0.25};
};

}  // namespace djcore
