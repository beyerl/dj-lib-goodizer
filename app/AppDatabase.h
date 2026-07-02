#pragma once

#include <memory>
#include <string>

#include "djcore/db/Database.h"

namespace djapp {

// Owns the per-user project database: a single SQLite file under the platform's
// app-data location, migrated and seeded with the default profiles on open. A
// file-backed (not in-memory) DB so worker threads can open their own WAL
// connection to the same file.
class AppDatabase {
 public:
  AppDatabase();

  djcore::Database& db() { return *db_; }
  const std::string& path() const { return path_; }

 private:
  std::string path_;
  std::unique_ptr<djcore::Database> db_;
};

}  // namespace djapp
