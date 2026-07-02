#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace djcore {

class DbError : public std::runtime_error {
 public:
  explicit DbError(const std::string& what) : std::runtime_error(what) {}
};

// Thin RAII wrapper over a prepared statement with a fluent bind API.
// Bind indices are 1-based (sqlite convention); column indices are 0-based.
class Statement {
 public:
  Statement(sqlite3* db, const std::string& sql);
  ~Statement();
  Statement(Statement&&) noexcept;
  Statement& operator=(Statement&&) noexcept;
  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;

  Statement& bind(int index, std::int64_t value);
  Statement& bind(int index, double value);
  Statement& bind(int index, const std::string& value);
  Statement& bindNull(int index);

  // Advances the statement. Returns true if a row is available, false when done.
  bool step();

  std::int64_t columnInt64(int col) const;
  double columnDouble(int col) const;
  std::string columnText(int col) const;
  bool columnIsNull(int col) const;

 private:
  sqlite3_stmt* stmt_ = nullptr;
};

// RAII sqlite3 connection. Opens with WAL journaling, a busy timeout (so
// concurrent writers retry instead of failing with "database is locked"), and
// foreign-key enforcement.
class Database {
 public:
  explicit Database(const std::string& path);  // ":memory:" for in-memory
  ~Database();
  Database(Database&&) noexcept;
  Database& operator=(Database&&) noexcept;
  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;

  void exec(const std::string& sql);
  Statement prepare(const std::string& sql);
  std::int64_t lastInsertRowId() const;

  int userVersion();
  void setUserVersion(int version);

  sqlite3* handle() const { return db_; }

 private:
  sqlite3* db_ = nullptr;
};

}  // namespace djcore
