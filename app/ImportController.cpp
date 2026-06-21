#include "ImportController.h"

#include <QDirIterator>
#include <QFileInfo>
#include <algorithm>
#include <ctime>
#include <vector>

#include "djcore/analysis/AnalysisPipeline.h"
#include "djcore/audio/AudioDecoder.h"
#include "djcore/db/AnalysisResultRepository.h"
#include "djcore/db/Database.h"
#include "djcore/db/TrackRepository.h"
#include "djcore/model/Track.h"

namespace djapp {
namespace {

bool isAudioFile(const QString& path) {
  static const QStringList exts = {"wav", "wave", "flac", "mp3", "aiff",
                                   "aif", "ogg", "aac",  "m4a", "alac"};
  return exts.contains(QFileInfo(path).suffix().toLower());
}

}  // namespace

ImportController::ImportController(std::string dbPath, QString folder, QObject* parent)
    : QObject(parent), dbPath_(std::move(dbPath)), folder_(std::move(folder)) {}

void ImportController::run() {
  // Discover candidate files.
  std::vector<QString> files;
  QDirIterator it(folder_, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QString p = it.next();
    if (isAudioFile(p)) files.push_back(p);
  }

  // Own connection on this worker thread (WAL allows concurrent UI reads).
  djcore::Database db(dbPath_);
  djcore::TrackRepository tracks(db);
  djcore::AnalysisResultRepository results(db);
  djcore::AnalysisPipeline pipeline;

  int imported = 0;
  int failed = 0;
  const int total = static_cast<int>(files.size());
  for (int i = 0; i < total; ++i) {
    const std::string path = files[static_cast<std::size_t>(i)].toStdString();
    emit progress(i, total, QFileInfo(files[static_cast<std::size_t>(i)]).fileName());
    try {
      auto decoder = djcore::openDecoder(path);
      djcore::Track t;
      t.sourcePath = path;
      t.format = decoder->format();
      t.importedAtUnix = static_cast<std::int64_t>(std::time(nullptr));
      const std::int64_t id = tracks.insert(t);

      djcore::AnalysisResult r = pipeline.analyze(*decoder);
      r.analyzedAtUnix = static_cast<std::int64_t>(std::time(nullptr));
      results.upsert(id, r);
      ++imported;
    } catch (...) {
      ++failed;
    }
  }
  emit progress(total, total, {});
  emit finished(imported, failed);
}

}  // namespace djapp
