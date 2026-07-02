#pragma once

#include <cstdint>
#include <string>

#include "djcore/model/Enums.h"

namespace djcore {

// One recorded operation, with before/after metric snapshots, for the
// persistent, exportable audit log (NFR-SAFE-3).
struct AuditLogEntry {
  std::int64_t id = 0;
  std::int64_t trackId = 0;
  std::int64_t jobId = 0;  // 0 when not part of a batch job
  OperationType operation = OperationType::None;
  std::string paramsJson;
  std::string beforeJson;
  std::string afterJson;
  std::int64_t timestampUnix = 0;
};

}  // namespace djcore
