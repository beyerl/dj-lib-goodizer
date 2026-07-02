#include "djcore/db/AuditLogRepository.h"

#include "djcore/db/Database.h"

namespace djcore {
namespace {

AuditLogEntry readRow(Statement& st) {
  AuditLogEntry e;
  e.id = st.columnInt64(0);
  e.trackId = st.columnInt64(1);
  e.jobId = st.columnInt64(2);
  e.operation = operationFromString(st.columnText(3));
  e.paramsJson = st.columnText(4);
  e.beforeJson = st.columnText(5);
  e.afterJson = st.columnText(6);
  e.timestampUnix = st.columnInt64(7);
  return e;
}

}  // namespace

std::int64_t AuditLogRepository::insert(const AuditLogEntry& entry) {
  Statement st = db_.prepare(
      "INSERT INTO audit_log_entry (track_id, job_id, operation, params_json, "
      "before_json, after_json, timestamp) VALUES (?,?,?,?,?,?,?);");
  st.bind(1, entry.trackId);
  st.bind(2, entry.jobId);
  st.bind(3, std::string(toString(entry.operation)));
  st.bind(4, entry.paramsJson);
  st.bind(5, entry.beforeJson);
  st.bind(6, entry.afterJson);
  st.bind(7, entry.timestampUnix);
  st.step();
  return db_.lastInsertRowId();
}

std::vector<AuditLogEntry> AuditLogRepository::all() {
  Statement st = db_.prepare(
      "SELECT id, track_id, job_id, operation, params_json, before_json, "
      "after_json, timestamp FROM audit_log_entry ORDER BY id;");
  std::vector<AuditLogEntry> out;
  while (st.step()) out.push_back(readRow(st));
  return out;
}

std::vector<AuditLogEntry> AuditLogRepository::forTrack(std::int64_t trackId) {
  Statement st = db_.prepare(
      "SELECT id, track_id, job_id, operation, params_json, before_json, "
      "after_json, timestamp FROM audit_log_entry WHERE track_id=? ORDER BY id;");
  st.bind(1, trackId);
  std::vector<AuditLogEntry> out;
  while (st.step()) out.push_back(readRow(st));
  return out;
}

std::int64_t AuditLogRepository::count() {
  Statement st = db_.prepare("SELECT COUNT(*) FROM audit_log_entry;");
  st.step();
  return st.columnInt64(0);
}

}  // namespace djcore
