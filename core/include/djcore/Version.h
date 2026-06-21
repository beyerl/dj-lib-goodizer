#pragma once

namespace djcore {

// Bumped whenever an analyzer's output semantics change, so persisted
// AnalysisResult rows can be invalidated and recomputed (see plan §2).
inline constexpr int kAnalyzerVersion = 1;

// Human-readable core library version string (e.g. "0.1.0").
const char* coreVersionString();

}  // namespace djcore
