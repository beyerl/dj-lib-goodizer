#pragma once

#include <cstdint>
#include <string>

#include "djcore/model/Enums.h"

namespace djcore {

// One row of the persistent, exportable processing log (NFR-SAFE-3): records
// what was done to each file with before/after metric snapshots for audit.
// Parameter and metric snapshots are stored as JSON blobs (db layer, M4).
struct AuditLogEntry {
  std::int64_t id = 0;
  std::int64_t trackId = 0;
  std::int64_t jobId = 0;
  OperationType operation = OperationType::ResampleTranscode;
  std::string paramsJson;          // operation parameters applied
  std::string beforeMetricsJson;   // AnalysisResult snapshot pre-op
  std::string afterMetricsJson;    // AnalysisResult snapshot post-op
  std::int64_t timestampUnix = 0;
};

}  // namespace djcore
