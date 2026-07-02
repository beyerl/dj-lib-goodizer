#pragma once

namespace djcore {

// Three-state flag model used across all feature areas (never binary
// pass/fail). Maps to UI colors: Ok=green, Review=yellow, OutsideTarget=red.
enum class FlagState { Ok, Review, OutsideTarget };

enum class JobStatus { Queued, Running, Paused, Complete, Failed, Cancelled };

// Standardizing operations are safe/default; PerceptualAltering operations
// (compression, width/balance) are opt-in and user-confirmed.
enum class OperationCategory { Standardizing, PerceptualAltering };

enum class OperationType {
  None,
  ResampleTranscode,       // Standardizing
  Dither,                  // Standardizing
  SilenceTrim,             // Standardizing
  GainNormalize,           // Standardizing
  Compression,             // PerceptualAltering
  WidthBalanceCorrection,  // PerceptualAltering
};

// Stable string form for persistence / audit log.
const char* toString(OperationType type);

}  // namespace djcore
