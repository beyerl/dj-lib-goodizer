#pragma once

namespace djcore {

// Tunable parameters shared by the analyzers. Defaults follow the DSP spec.
struct AnalysisConfig {
  double silenceThresholdDb = -60.0;  // level below which audio counts as silent
  int silenceMinDurationMs = 100;     // ignore silent runs shorter than this
  double truePeakCeilingDbtp = -1.0;  // gain-limit ceiling (used by processing)
};

}  // namespace djcore
