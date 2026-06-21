#include "AppDatabase.h"

#include <QDir>
#include <QStandardPaths>

#include "djcore/db/ProfileRepository.h"
#include "djcore/db/Schema.h"

namespace djapp {

AppDatabase::AppDatabase() {
  const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(dir);
  path_ = QDir(dir).filePath("library.db").toStdString();

  db_ = std::make_unique<djcore::Database>(path_);
  djcore::migrate(*db_);

  djcore::ProfileRepository profiles(*db_);
  profiles.seedDefaultsIfEmpty();
}

}  // namespace djapp
