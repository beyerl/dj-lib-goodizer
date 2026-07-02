#include "ImportController.h"

#include <ctime>
#include <utility>
#include <vector>

#include <QDirIterator>
#include <QFileInfo>

#include "djcore/analysis/AnalysisPipeline.h"
#include "djcore/audio/AudioDecoder.h"
#include "djcore/db/AnalysisResultRepository.h"
#include "djcore/db/Database.h"
#include "djcore/db/TrackRepository.h"
#include "djcore/model/Track.h"

namespace djapp {

ImportController::ImportController(std::string dbPath, QString folder,
                                  std::shared_ptr<std::atomic<bool>> cancel,
                                  QObject* parent)
    : Worker(parent),
      dbPath_(std::move(dbPath)),
      folder_(std::move(folder)),
      cancel_(std::move(cancel)) {}

void ImportController::run() {
  std::vector<QString> files;
  QDirIterator it(folder_, QDir::Files, QDirIterator::Subdirectories);
  while (it.hasNext()) {
    const QString p = it.next();
    if (djcore::canDecode(p.toStdString())) files.push_back(p);
  }

  djcore::Database db(dbPath_);  // own WAL connection on this thread
  djcore::TrackRepository tracks(db);
  djcore::AnalysisResultRepository results(db);
  djcore::AnalysisPipeline pipeline;

  int imported = 0, failed = 0;
  const int total = static_cast<int>(files.size());
  for (int i = 0; i < total; ++i) {
    if (cancel_ && cancel_->load()) break;
    emit progress(i, total, QFileInfo(files[static_cast<std::size_t>(i)]).fileName());
    try {
      const std::string path = files[static_cast<std::size_t>(i)].toStdString();
      auto decoder = djcore::openDecoder(path);
      djcore::Track t;
      t.sourcePath = path;
      t.format = decoder->format();
      t.importedAtUnix = static_cast<std::int64_t>(std::time(nullptr));
      const std::int64_t id = tracks.insert(t);

      djcore::AnalysisResult r = pipeline.analyze(*decoder);
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
