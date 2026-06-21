#pragma once

#include <cstdint>
#include <vector>

#include "djcore/model/AuditLogEntry.h"

namespace djcore {

class Database;

// Persistence for the processing/audit log (NFR-SAFE-3): an exportable record
// of every operation performed, with before/after metric snapshots.
class AuditLogRepository {
 public:
  explicit AuditLogRepository(Database& db) : db_(db) {}

  std::int64_t insert(const AuditLogEntry& entry);
  std::vector<AuditLogEntry> byTrack(std::int64_t trackId);
  std::int64_t count();

 private:
  Database& db_;
};

}  // namespace djcore
