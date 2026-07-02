#include "djcore/db/Schema.h"

#include "djcore/db/Database.h"

namespace djcore {
namespace {

// Schema v1: the minimum needed for the M2 vertical slice — imported tracks and
// the processing audit log. Later milestones add analysis_result, target_profile,
// and job tables via additional migration steps below.
const char* kSchemaV1 = R"SQL(
CREATE TABLE IF NOT EXISTS track (
  id           INTEGER PRIMARY KEY,
  source_path  TEXT NOT NULL,
  output_path  TEXT,
  container    TEXT,
  codec        TEXT,
  sample_rate  INTEGER,
  bit_depth    INTEGER,
  channels     INTEGER,
  duration_ms  INTEGER,
  source_lossy INTEGER NOT NULL DEFAULT 0,
  imported_at  INTEGER
);

CREATE TABLE IF NOT EXISTS audit_log_entry (
  id          INTEGER PRIMARY KEY,
  track_id    INTEGER,
  job_id      INTEGER NOT NULL DEFAULT 0,
  operation   TEXT NOT NULL,
  params_json TEXT,
  before_json TEXT,
  after_json  TEXT,
  timestamp   INTEGER,
  FOREIGN KEY(track_id) REFERENCES track(id)
);

CREATE INDEX IF NOT EXISTS idx_audit_track ON audit_log_entry(track_id);
)SQL";

}  // namespace

void migrate(Database& db) {
  int v = db.userVersion();
  if (v < 1) {
    db.exec(kSchemaV1);
    db.setUserVersion(1);
    v = 1;
  }
  // Future migrations:
  //   if (v < 2) { db.exec(kSchemaV2); db.setUserVersion(2); v = 2; }
}

}  // namespace djcore
