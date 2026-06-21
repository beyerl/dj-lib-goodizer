#pragma once

#include <cstdint>
#include <string>

#include "djcore/model/AnalysisResult.h"
#include "djcore/model/TargetProfile.h"
#include "djcore/model/Track.h"

namespace djcore {

// What a processing run would change for one track — the dry-run report
// (NFR-SAFE-2) and the basis for the audit log (NFR-SAFE-3).
struct ProcessingChange {
  bool resampled = false;
  int fromRate = 0, toRate = 0;
  bool requantized = false;
  int fromBits = 0, toBits = 0;
  bool containerChanged = false;
  std::string fromContainer, toContainer;
  bool trimmed = false;
  std::int64_t trimLeadMs = 0, trimTailMs = 0;
  bool gainApplied = false;
  double gainDb = 0.0;
  bool gainLimitedForTruePeak = false;  // gain reduced to avoid clipping
  bool passthrough = false;             // already conforms; nothing to change
  std::int64_t outDurationMs = 0;
};

struct ProcessingOptions {
  bool applyTrim = true;
  bool applyGain = true;
  // Perceptual-altering ops (compression, width/balance) are opt-in and live in
  // Milestone 8; they are intentionally absent from the standardizing chain.
};

// Applies the standardizing operation chain (trim -> gain-only normalize ->
// resample -> encode) to bring a track into conformance with a TargetProfile,
// writing to a separate output path by default (plan §3, NFR-SAFE-1).
class ProcessingEngine {
 public:
  explicit ProcessingEngine(TargetProfile profile) : profile_(std::move(profile)) {}

  // Compute what would change without decoding/writing (dry run).
  ProcessingChange plan(const Track& track, const AnalysisResult& analysis,
                        const ProcessingOptions& options = {}) const;

  // Execute the chain and write the conditioned file to outputPath.
  ProcessingChange process(const Track& track, const AnalysisResult& analysis,
                           const std::string& outputPath,
                           const ProcessingOptions& options = {}) const;

  const TargetProfile& profile() const { return profile_; }

 private:
  TargetProfile profile_;
};

}  // namespace djcore
