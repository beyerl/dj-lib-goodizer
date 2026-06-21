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

// RAII prepared statement with a small fluent binding/column API.
class Statement {
 public:
  Statement(sqlite3* db, const std::string& sql);
  ~Statement();
  Statement(Statement&&) noexcept;
  Statement& operator=(Statement&&) noexcept;
  Statement(const Statement&) = delete;
  Statement& operator=(const Statement&) = delete;

  // 1-based bind index.
  Statement& bind(int i, std::int64_t v);
  Statement& bind(int i, double v);
  Statement& bind(int i, const std::string& v);
  Statement& bindNull(int i);

  // Advance one row; returns true if a row is available, false when done.
  bool step();

  // 0-based column accessors.
  std::int64_t columnInt64(int c) const;
  double columnDouble(int c) const;
  std::string columnText(int c) const;
  bool columnIsNull(int c) const;

  void reset();

 private:
  sqlite3* db_ = nullptr;
  sqlite3_stmt* stmt_ = nullptr;
};

// RAII connection to a SQLite database (file path or ":memory:"). Opens with
// WAL journaling and foreign keys enabled (plan §5).
class Database {
 public:
  explicit Database(const std::string& path);
  ~Database();
  Database(Database&&) noexcept;
  Database& operator=(Database&&) noexcept;
  Database(const Database&) = delete;
  Database& operator=(const Database&) = delete;

  void exec(const std::string& sql);
  Statement prepare(const std::string& sql);
  std::int64_t lastInsertRowId() const;

  sqlite3* handle() const { return db_; }

 private:
  sqlite3* db_ = nullptr;
};

}  // namespace djcore
