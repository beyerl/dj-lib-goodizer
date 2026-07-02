#pragma once

namespace djcore {

class Database;

// Current schema version. Bumped as migrations are added in later milestones.
inline constexpr int kSchemaVersion = 1;

// Applies any pending migrations, stepping PRAGMA user_version. Idempotent;
// safe to run on every open.
void migrate(Database& db);

}  // namespace djcore
