#pragma once

#include <cstdint>
#include <optional>

namespace djcore {

// Structured metrics produced by the analysis engine for a single track
// (data model, spec Table 11 / §7). Stereo-only metrics are optional so mono
// files leave them unset rather than carrying meaningless values.
struct AnalysisResult {
  // Feature Area B — silence boundaries.
  std::int64_t leadingSilenceMs = 0;
  std::int64_t trailingSilenceMs = 0;
  bool silenceAmbiguous = false;          // FR-SIL-7: low-level intro/outro

  // Feature Area C — loudness & dynamic range (kept separate, never composite).
  double integratedLufs = 0.0;            // FR-DR-1
  double truePeakDbtp = 0.0;              // FR-DR-8
  double crestFactor = 0.0;               // FR-DR-2
  double drValue = 0.0;                   // FR-DR-2 (TT DR-meter style)

  // Feature Area D — mono compatibility (stereo only).
  std::optional<double> phaseCorrelation;     // FR-MONO-1: -1..+1
  std::optional<double> monoFolddownDeltaDb;  // FR-MONO-2: level loss on L+R sum

  // Feature Area E — stereo width / balance (stereo only).
  std::optional<double> stereoWidth;          // FR-WID-1: mid/side derived
  std::optional<double> lrBalanceDb;          // FR-WID-2: +ve => right louder

  int analyzerVersion = 0;                 // see Version.h / kAnalyzerVersion
  std::int64_t analyzedAtUnix = 0;
};

}  // namespace djcore
