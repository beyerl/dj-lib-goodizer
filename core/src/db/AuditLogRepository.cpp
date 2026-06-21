#include "djcore/db/AuditLogRepository.h"

#include "djcore/db/Database.h"

namespace djcore {

std::int64_t AuditLogRepository::insert(const AuditLogEntry& e) {
  Statement s = db_.prepare(
      "INSERT INTO audit_log_entry (track_id, job_id, operation, params_json, "
      "before_json, after_json, timestamp) VALUES (?,?,?,?,?,?,?);");
  s.bind(1, e.trackId);
  if (e.jobId > 0) s.bind(2, e.jobId); else s.bindNull(2);
  s.bind(3, static_cast<std::int64_t>(e.operation));
  s.bind(4, e.paramsJson);
  s.bind(5, e.beforeMetricsJson);
  s.bind(6, e.afterMetricsJson);
  s.bind(7, e.timestampUnix);
  s.step();
  return db_.lastInsertRowId();
}

std::vector<AuditLogEntry> AuditLogRepository::byTrack(std::int64_t trackId) {
  Statement s = db_.prepare(
      "SELECT id, track_id, job_id, operation, params_json, before_json, "
      "after_json, timestamp FROM audit_log_entry WHERE track_id=? ORDER BY id;");
  s.bind(1, trackId);
  std::vector<AuditLogEntry> out;
  while (s.step()) {
    AuditLogEntry e;
    e.id = s.columnInt64(0);
    e.trackId = s.columnInt64(1);
    e.jobId = s.columnIsNull(2) ? 0 : s.columnInt64(2);
    e.operation = static_cast<OperationType>(s.columnInt64(3));
    e.paramsJson = s.columnText(4);
    e.beforeMetricsJson = s.columnText(5);
    e.afterMetricsJson = s.columnText(6);
    e.timestampUnix = s.columnInt64(7);
    out.push_back(e);
  }
  return out;
}

std::int64_t AuditLogRepository::count() {
  Statement s = db_.prepare("SELECT COUNT(*) FROM audit_log_entry;");
  s.step();
  return s.columnInt64(0);
}

}  // namespace djcore
