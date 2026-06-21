#pragma once

namespace djcore {

// Tunables for the analysis pass. Defaults are conservative; the UI exposes
// these via the active TargetProfile (spec §8.1 open Q2 — defaults are not
// load-bearing).
struct AnalysisConfig {
  double silenceThresholdDb = -60.0;  // level at/below which audio is "silent"
  int silenceMinDurationMs = 100;     // min run to qualify as a silent region
};

}  // namespace djcore
