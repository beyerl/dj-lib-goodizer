#pragma once

#include <cstdint>
#include <optional>

namespace djcore {

// Per-track metrics produced by the analysis pipeline. Stereo-only metrics are
// optional (unset for mono files). Populated incrementally as analyzers land in
// later milestones; a default-constructed result is the "not yet analyzed" state.
struct AnalysisResult {
  // Feature Area B — silence boundaries.
  std::int64_t leadingSilenceMs = 0;
  std::int64_t trailingSilenceMs = 0;
  bool silenceAmbiguous = false;

  // Feature Area C — loudness & dynamics.
  double integratedLufs = 0.0;
  double truePeakDbtp = 0.0;
  double crestFactor = 0.0;
  double drValue = 0.0;

  // Feature Area D — mono compatibility (stereo only).
  std::optional<double> phaseCorrelation;
  std::optional<double> monoFolddownDeltaDb;

  // Feature Area E — stereo width & balance (stereo only).
  std::optional<double> stereoWidth;
  std::optional<double> lrBalanceDb;

  int analyzerVersion = 0;
  std::int64_t analyzedAtUnix = 0;
};

}  // namespace djcore
