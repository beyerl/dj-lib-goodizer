#pragma once

#include <cstdint>
#include <vector>

#include "djcore/model/AuditLogEntry.h"

namespace djcore {

class Database;

// Append-only store for the processing audit log.
class AuditLogRepository {
 public:
  explicit AuditLogRepository(Database& db) : db_(db) {}

  std::int64_t insert(const AuditLogEntry& entry);  // returns new id
  std::vector<AuditLogEntry> all();
  std::vector<AuditLogEntry> forTrack(std::int64_t trackId);
  std::int64_t count();

 private:
  Database& db_;
};

}  // namespace djcore
