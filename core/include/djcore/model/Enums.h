#pragma once

namespace djcore {

// Three-state flag model used consistently across all feature areas (spec §5.2):
// never a misleading binary pass/fail for continuous metrics.
enum class FlagState {
  Ok,
  Review,        // borderline / ambiguous
  OutsideTarget  // clearly outside the configured tolerance
};

// Lifecycle of a batch analysis or processing job (matches ProcessingJob model).
enum class JobStatus {
  Queued,
  Running,
  Paused,
  Complete,
  Failed,
  Cancelled
};

// Processing operations are split into two categories that are enforced in
// code, not just in the UI (spec §2.4, §5.2):
//  - Standardizing      : safe, default-on corrective/standardizing ops.
//  - PerceptualAltering : opt-in, explicitly confirmed, dynamics/image-altering.
enum class OperationCategory {
  Standardizing,
  PerceptualAltering
};

enum class OperationType {
  ResampleTranscode,        // format / sample-rate / bit-depth (Standardizing)
  Dither,                   // bit-depth reduction              (Standardizing)
  SilenceTrim,              // lead/trail silence trim          (Standardizing)
  GainNormalize,            // gain-only loudness normalization (Standardizing)
  Compression,              // dynamics compression/limiting    (PerceptualAltering)
  WidthBalanceCorrection    // mid/side + channel-balance       (PerceptualAltering)
};

}  // namespace djcore
