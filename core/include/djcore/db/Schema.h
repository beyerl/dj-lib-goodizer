#pragma once

namespace djcore {

class Database;

// Current schema version. Bumped as migrations are added.
inline constexpr int kSchemaVersion = 2;

// Applies any pending migrations, stepping PRAGMA user_version. Idempotent;
// safe to run on every open.
void migrate(Database& db);

}  // namespace djcore
