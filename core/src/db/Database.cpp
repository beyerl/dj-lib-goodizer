#include "djcore/db/Database.h"

#include <sqlite3.h>

#include <string>
#include <utility>

namespace djcore {

// ---- Statement --------------------------------------------------------------

Statement::Statement(sqlite3* db, const std::string& sql) {
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
    std::string msg = sqlite3_errmsg(db);
    throw DbError("prepare failed: " + msg + "  [SQL: " + sql + "]");
  }
}

Statement::~Statement() {
  if (stmt_) sqlite3_finalize(stmt_);
}

Statement::Statement(Statement&& other) noexcept : stmt_(other.stmt_) {
  other.stmt_ = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept {
  if (this != &other) {
    if (stmt_) sqlite3_finalize(stmt_);
    stmt_ = other.stmt_;
    other.stmt_ = nullptr;
  }
  return *this;
}

Statement& Statement::bind(int index, std::int64_t value) {
  sqlite3_bind_int64(stmt_, index, value);
  return *this;
}
Statement& Statement::bind(int index, double value) {
  sqlite3_bind_double(stmt_, index, value);
  return *this;
}
Statement& Statement::bind(int index, const std::string& value) {
  sqlite3_bind_text(stmt_, index, value.c_str(), static_cast<int>(value.size()),
                    SQLITE_TRANSIENT);
  return *this;
}
Statement& Statement::bindNull(int index) {
  sqlite3_bind_null(stmt_, index);
  return *this;
}

bool Statement::step() {
  const int rc = sqlite3_step(stmt_);
  if (rc == SQLITE_ROW) return true;
  if (rc == SQLITE_DONE) return false;
  throw DbError(std::string("step failed: ") +
                sqlite3_errmsg(sqlite3_db_handle(stmt_)));
}

std::int64_t Statement::columnInt64(int col) const {
  return sqlite3_column_int64(stmt_, col);
}
double Statement::columnDouble(int col) const {
  return sqlite3_column_double(stmt_, col);
}
std::string Statement::columnText(int col) const {
  const unsigned char* text = sqlite3_column_text(stmt_, col);
  if (!text) return {};
  return reinterpret_cast<const char*>(text);
}
bool Statement::columnIsNull(int col) const {
  return sqlite3_column_type(stmt_, col) == SQLITE_NULL;
}

// ---- Database ---------------------------------------------------------------

Database::Database(const std::string& path) {
  if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
    std::string msg = db_ ? sqlite3_errmsg(db_) : "out of memory";
    if (db_) sqlite3_close(db_);
    db_ = nullptr;
    throw DbError("cannot open database '" + path + "': " + msg);
  }
  // busy_timeout MUST be set before switching to WAL: changing journal_mode
  // needs a brief exclusive lock, and if two connections open the same file at
  // once the loser gets SQLITE_BUSY. With the busy timeout installed first, that
  // pragma waits/retries instead of throwing "database is locked". Then WAL for
  // concurrent readers/writers, and foreign-key enforcement.
  exec("PRAGMA busy_timeout=5000;");
  exec("PRAGMA journal_mode=WAL;");
  exec("PRAGMA foreign_keys=ON;");
}

Database::~Database() {
  if (db_) sqlite3_close(db_);
}

Database::Database(Database&& other) noexcept : db_(other.db_) {
  other.db_ = nullptr;
}

Database& Database::operator=(Database&& other) noexcept {
  if (this != &other) {
    if (db_) sqlite3_close(db_);
    db_ = other.db_;
    other.db_ = nullptr;
  }
  return *this;
}

void Database::exec(const std::string& sql) {
  char* err = nullptr;
  if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
    std::string msg = err ? err : "unknown error";
    sqlite3_free(err);
    throw DbError("exec failed: " + msg + "  [SQL: " + sql + "]");
  }
}

Statement Database::prepare(const std::string& sql) { return Statement(db_, sql); }

std::int64_t Database::lastInsertRowId() const {
  return sqlite3_last_insert_rowid(db_);
}

int Database::userVersion() {
  Statement st = prepare("PRAGMA user_version;");
  if (st.step()) return static_cast<int>(st.columnInt64(0));
  return 0;
}

void Database::setUserVersion(int version) {
  exec("PRAGMA user_version=" + std::to_string(version) + ";");
}

}  // namespace djcore
