#include "djcore/db/Schema.h"

#include "djcore/db/Database.h"

namespace djcore {
namespace {

// Schema mirrors the conceptual data model (spec §7 / Table 11). JSON blobs hold
// structured sub-objects (silence policy, tolerance bands, metric snapshots).
const char* kSchemaV1 = R"sql(
CREATE TABLE IF NOT EXISTS track (
  id            INTEGER PRIMARY KEY,
  source_path   TEXT NOT NULL,
  output_path   TEXT,
  container     TEXT,
  codec         TEXT,
  sample_rate   INTEGER,
  bit_depth     INTEGER,
  channels      INTEGER,
  duration_ms   INTEGER,
  source_lossy  INTEGER DEFAULT 0,
  imported_at   INTEGER
);

CREATE TABLE IF NOT EXISTS analysis_result (
  id                   INTEGER PRIMARY KEY,
  track_id             INTEGER NOT NULL REFERENCES track(id) ON DELETE CASCADE,
  leading_silence_ms   INTEGER,
  trailing_silence_ms  INTEGER,
  silence_ambiguous    INTEGER,
  integrated_lufs      REAL,
  true_peak_dbtp       REAL,
  crest_factor         REAL,
  dr_value             REAL,
  phase_correlation    REAL,
  mono_folddown_db     REAL,
  stereo_width         REAL,
  lr_balance_db        REAL,
  analyzer_version     INTEGER,
  analyzed_at          INTEGER,
  UNIQUE(track_id)
);

CREATE TABLE IF NOT EXISTS target_profile (
  id                  INTEGER PRIMARY KEY,
  name                TEXT NOT NULL UNIQUE,
  container           TEXT,
  sample_rate         INTEGER,
  bit_depth           INTEGER,
  silence_json        TEXT,
  loudness_target     REAL,
  dr_tolerance_json   TEXT,
  mono_threshold      REAL,
  width_tolerance_json TEXT
);

CREATE TABLE IF NOT EXISTS processing_job (
  id           INTEGER PRIMARY KEY,
  profile_id   INTEGER REFERENCES target_profile(id),
  status       INTEGER,
  options_json TEXT,
  started_at   INTEGER,
  ended_at     INTEGER
);

CREATE TABLE IF NOT EXISTS processing_job_track (
  job_id    INTEGER NOT NULL REFERENCES processing_job(id) ON DELETE CASCADE,
  track_id  INTEGER NOT NULL REFERENCES track(id) ON DELETE CASCADE,
  status    INTEGER,
  error     TEXT,
  PRIMARY KEY (job_id, track_id)
);

CREATE TABLE IF NOT EXISTS audit_log_entry (
  id             INTEGER PRIMARY KEY,
  track_id       INTEGER REFERENCES track(id) ON DELETE CASCADE,
  job_id         INTEGER REFERENCES processing_job(id) ON DELETE SET NULL,
  operation      INTEGER,
  params_json    TEXT,
  before_json    TEXT,
  after_json     TEXT,
  timestamp      INTEGER
);

-- Indices for sort/filter responsiveness on large libraries (NFR-UX-3/PERF-1).
CREATE INDEX IF NOT EXISTS idx_ar_track   ON analysis_result(track_id);
CREATE INDEX IF NOT EXISTS idx_ar_lufs    ON analysis_result(integrated_lufs);
CREATE INDEX IF NOT EXISTS idx_ar_dr      ON analysis_result(dr_value);
CREATE INDEX IF NOT EXISTS idx_ar_mono    ON analysis_result(phase_correlation);
CREATE INDEX IF NOT EXISTS idx_ar_width   ON analysis_result(stereo_width);
CREATE INDEX IF NOT EXISTS idx_audit_track ON audit_log_entry(track_id);
)sql";

}  // namespace

void migrate(Database& db) {
  Statement getVer = db.prepare("PRAGMA user_version;");
  getVer.step();
  const int current = static_cast<int>(getVer.columnInt64(0));

  if (current < 1) {
    db.exec("BEGIN;");
    db.exec(kSchemaV1);
    db.exec("PRAGMA user_version=1;");
    db.exec("COMMIT;");
  }
  // Future: if (current < 2) { ... } etc.
}

}  // namespace djcore
