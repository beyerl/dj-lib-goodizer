#include "djcore/db/Database.h"

#include <sqlite3.h>

#include <utility>

namespace djcore {

// ---- Statement --------------------------------------------------------------

Statement::Statement(sqlite3* db, const std::string& sql) : db_(db) {
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
    throw DbError(std::string("prepare failed: ") + sqlite3_errmsg(db));
  }
}

Statement::~Statement() {
  if (stmt_) sqlite3_finalize(stmt_);
}

Statement::Statement(Statement&& o) noexcept : db_(o.db_), stmt_(o.stmt_) {
  o.stmt_ = nullptr;
}

Statement& Statement::operator=(Statement&& o) noexcept {
  if (this != &o) {
    if (stmt_) sqlite3_finalize(stmt_);
    db_ = o.db_;
    stmt_ = o.stmt_;
    o.stmt_ = nullptr;
  }
  return *this;
}

Statement& Statement::bind(int i, std::int64_t v) {
  sqlite3_bind_int64(stmt_, i, v);
  return *this;
}
Statement& Statement::bind(int i, double v) {
  sqlite3_bind_double(stmt_, i, v);
  return *this;
}
Statement& Statement::bind(int i, const std::string& v) {
  sqlite3_bind_text(stmt_, i, v.c_str(), -1, SQLITE_TRANSIENT);
  return *this;
}
Statement& Statement::bindNull(int i) {
  sqlite3_bind_null(stmt_, i);
  return *this;
}

bool Statement::step() {
  const int rc = sqlite3_step(stmt_);
  if (rc == SQLITE_ROW) return true;
  if (rc == SQLITE_DONE) return false;
  throw DbError(std::string("step failed: ") + sqlite3_errmsg(db_));
}

std::int64_t Statement::columnInt64(int c) const {
  return sqlite3_column_int64(stmt_, c);
}
double Statement::columnDouble(int c) const {
  return sqlite3_column_double(stmt_, c);
}
std::string Statement::columnText(int c) const {
  const unsigned char* t = sqlite3_column_text(stmt_, c);
  return t ? reinterpret_cast<const char*>(t) : std::string();
}
bool Statement::columnIsNull(int c) const {
  return sqlite3_column_type(stmt_, c) == SQLITE_NULL;
}

void Statement::reset() {
  sqlite3_reset(stmt_);
  sqlite3_clear_bindings(stmt_);
}

// ---- Database ---------------------------------------------------------------

Database::Database(const std::string& path) {
  if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
    const std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
    if (db_) sqlite3_close(db_);
    throw DbError("cannot open database: " + msg);
  }
  exec("PRAGMA journal_mode=WAL;");
  exec("PRAGMA foreign_keys=ON;");
}

Database::~Database() {
  if (db_) sqlite3_close(db_);
}

Database::Database(Database&& o) noexcept : db_(o.db_) { o.db_ = nullptr; }

Database& Database::operator=(Database&& o) noexcept {
  if (this != &o) {
    if (db_) sqlite3_close(db_);
    db_ = o.db_;
    o.db_ = nullptr;
  }
  return *this;
}

void Database::exec(const std::string& sql) {
  char* err = nullptr;
  if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
    const std::string msg = err ? err : "unknown error";
    sqlite3_free(err);
    throw DbError("exec failed: " + msg);
  }
}

Statement Database::prepare(const std::string& sql) { return Statement(db_, sql); }

std::int64_t Database::lastInsertRowId() const {
  return sqlite3_last_insert_rowid(db_);
}

}  // namespace djcore
