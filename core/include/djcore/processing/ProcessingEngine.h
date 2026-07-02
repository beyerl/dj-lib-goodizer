#pragma once

#include <cstdint>
#include <string>

#include "djcore/model/AnalysisResult.h"
#include "djcore/model/TargetProfile.h"
#include "djcore/model/Track.h"

namespace djcore {

// What a conform would change (dry-run) or did change (after process()).
struct ProcessingChange {
  bool resampled = false;
  bool requantized = false;
  bool containerChanged = false;
  bool trimmed = false;
  bool gainApplied = false;
  bool gainLimitedForTruePeak = false;
  bool passthrough = false;  // already conforming; nothing to do

  int fromRate = 0, toRate = 0;
  int fromBits = 0, toBits = 0;
  std::string fromContainer, toContainer;
  std::int64_t trimLeadMs = 0, trimTailMs = 0;
  double gainDb = 0.0;
  std::int64_t outDurationMs = 0;
};

// Which optional standardizing steps to apply. Grows in later milestones
// (trim in M5, gain in M6). Format/rate/depth conforming is always applied.
struct ProcessingOptions {
  bool applyTrim = false;
  bool applyGain = false;
};

// Brings a track into conformance with a target profile. plan() reports what
// would change without touching disk (dry-run / NFR-SAFE-2); process() writes
// the conformed file to outputPath (never overwrites the source).
class ProcessingEngine {
 public:
  explicit ProcessingEngine(TargetProfile profile);

  ProcessingChange plan(const Track& track, const AnalysisResult& analysis,
                        const ProcessingOptions& options = {}) const;

  ProcessingChange process(const Track& track, const AnalysisResult& analysis,
                           const std::string& outputPath,
                           const ProcessingOptions& options = {}) const;

  const TargetProfile& profile() const { return profile_; }

 private:
  TargetProfile profile_;
};

}  // namespace djcore
