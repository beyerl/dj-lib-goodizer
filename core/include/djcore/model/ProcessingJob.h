#pragma once

#include <cstdint>
#include <vector>

#include "djcore/model/Enums.h"

namespace djcore {

// Per-track status within a job, enabling crash-safe resume (NFR-SAFE-4):
// completed tracks are skipped on resume; partials are rolled back.
struct ProcessingJobTrack {
  std::int64_t trackId = 0;
  JobStatus status = JobStatus::Queued;
  std::string error;
};

// Opt-in switches for perceptual-altering operations, off by default
// (FR-DR-6, FR-WID-6); in-place processing is also explicitly opt-in.
struct ProcessingJobOptions {
  bool inPlace = false;
  bool enableCompression = false;
  bool enableWidthBalance = false;
  bool dryRun = false;            // NFR-SAFE-2: plan-only, write nothing
};

struct ProcessingJob {
  std::int64_t id = 0;
  std::int64_t profileId = 0;
  JobStatus status = JobStatus::Queued;
  ProcessingJobOptions options;
  std::vector<ProcessingJobTrack> tracks;
  std::int64_t startedAtUnix = 0;
  std::int64_t endedAtUnix = 0;
};

}  // namespace djcore
