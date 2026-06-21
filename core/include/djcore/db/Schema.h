#pragma once

namespace djcore {

class Database;

// Current schema version. Bump when adding a migration step.
inline constexpr int kSchemaVersion = 1;

// Creates/upgrades all tables and indices to kSchemaVersion. Idempotent and
// safe to call on every open (plan §5 — schema_version drives migrations).
void migrate(Database& db);

}  // namespace djcore
