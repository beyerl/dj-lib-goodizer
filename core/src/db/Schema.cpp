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

// Schema v2: per-track analysis metrics and named target profiles.
const char* kSchemaV2 = R"SQL(
CREATE TABLE IF NOT EXISTS analysis_result (
  id                  INTEGER PRIMARY KEY,
  track_id            INTEGER NOT NULL,
  leading_silence_ms  INTEGER,
  trailing_silence_ms INTEGER,
  silence_ambiguous   INTEGER,
  integrated_lufs     REAL,
  true_peak_dbtp      REAL,
  crest_factor        REAL,
  dr_value            REAL,
  phase_correlation   REAL,
  mono_folddown_db    REAL,
  stereo_width        REAL,
  lr_balance_db       REAL,
  analyzer_version    INTEGER,
  analyzed_at         INTEGER,
  UNIQUE(track_id),
  FOREIGN KEY(track_id) REFERENCES track(id)
);

CREATE TABLE IF NOT EXISTS target_profile (
  id                   INTEGER PRIMARY KEY,
  name                 TEXT UNIQUE NOT NULL,
  container            TEXT,
  sample_rate          INTEGER,
  bit_depth            INTEGER,
  silence_threshold_db REAL,
  silence_min_ms       INTEGER,
  silence_lead_in_ms   INTEGER,
  loudness_target      REAL,
  dr_target            REAL,
  dr_plusminus         REAL,
  width_target         REAL,
  width_plusminus      REAL,
  mono_threshold       REAL
);

CREATE INDEX IF NOT EXISTS idx_analysis_track ON analysis_result(track_id);
)SQL";

}  // namespace

void migrate(Database& db) {
  int v = db.userVersion();
  if (v < 1) {
    db.exec(kSchemaV1);
    db.setUserVersion(1);
    v = 1;
  }
  if (v < 2) {
    db.exec(kSchemaV2);
    db.setUserVersion(2);
    v = 2;
  }
}

}  // namespace djcore
