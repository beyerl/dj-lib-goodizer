#include "AppDatabase.h"

#include <QDir>
#include <QStandardPaths>

#include "djcore/db/ProfileRepository.h"
#include "djcore/db/Schema.h"

namespace djapp {

AppDatabase::AppDatabase() {
#ifdef __EMSCRIPTEN__
  // The browser has no persistent app-data directory and the import worker
  // thread is disabled in the WASM build, so a single in-memory database is
  // sufficient and avoids touching the virtual filesystem.
  path_ = ":memory:";
#else
  const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(dir);
  path_ = QDir(dir).filePath("library.db").toStdString();
#endif

  db_ = std::make_unique<djcore::Database>(path_);
  djcore::migrate(*db_);

  djcore::ProfileRepository profiles(*db_);
  profiles.seedDefaultsIfEmpty();
}

}  // namespace djapp
