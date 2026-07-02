#include "ProcessController.h"

#include <ctime>
#include <string>
#include <utility>

#include <QDir>
#include <QFileInfo>

#include "djcore/db/AnalysisResultRepository.h"
#include "djcore/db/AuditLogRepository.h"
#include "djcore/db/Database.h"
#include "djcore/db/ProfileRepository.h"
#include "djcore/db/TrackRepository.h"
#include "djcore/processing/ProcessingEngine.h"
#include "djcore/profile/DefaultProfiles.h"

namespace djapp {

ProcessController::ProcessController(std::string dbPath, QString profileName,
                                     QString outDir, bool applyGain, bool applyTrim,
                                     std::shared_ptr<std::atomic<bool>> cancel,
                                     QObject* parent)
    : Worker(parent),
      dbPath_(std::move(dbPath)),
      profileName_(std::move(profileName)),
      outDir_(std::move(outDir)),
      applyGain_(applyGain),
      applyTrim_(applyTrim),
      cancel_(std::move(cancel)) {}

void ProcessController::run() {
  djcore::Database db(dbPath_);
  djcore::ProfileRepository profiles(db);
  auto profile = profiles.getByName(profileName_.toStdString());
  if (!profile) profile = djcore::defaultProfileByName(profileName_.toStdString());
  if (!profile) {
    emit finished(0, 0);
    return;
  }

  djcore::TrackRepository tracks(db);
  djcore::AuditLogRepository audit(db);
  djcore::AnalysisResultRepository results(db);
  djcore::ProcessingEngine engine(*profile);
  djcore::ProcessingOptions options;
  options.applyGain = applyGain_;
  options.applyTrim = applyTrim_;

  QDir().mkpath(outDir_);
  const auto all = tracks.all();
  int processed = 0, failed = 0;
  const int total = static_cast<int>(all.size());
  for (int i = 0; i < total; ++i) {
    if (cancel_ && cancel_->load()) break;
    const auto& t = all[static_cast<std::size_t>(i)];
    const QFileInfo srcInfo(QString::fromStdString(t.sourcePath));
    emit progress(i, total, srcInfo.fileName());
    try {
      const QString outPath = QDir(outDir_).filePath(
          srcInfo.completeBaseName() + "." + QString::fromStdString(profile->container));

      djcore::AnalysisResult analysis;
      if (auto r = results.getByTrack(t.id)) analysis = *r;

      djcore::ProcessingChange ch =
          engine.process(t, analysis, outPath.toStdString(), options);
      tracks.setOutputPath(t.id, outPath.toStdString());

      djcore::OperationType op = djcore::OperationType::None;
      if (ch.gainApplied)
        op = djcore::OperationType::GainNormalize;
      else if (ch.resampled || ch.requantized || ch.containerChanged)
        op = djcore::OperationType::ResampleTranscode;
      else if (ch.trimmed)
        op = djcore::OperationType::SilenceTrim;

      djcore::AuditLogEntry entry;
      entry.trackId = t.id;
      entry.operation = op;
      entry.beforeJson = "{\"sampleRate\":" + std::to_string(ch.fromRate) +
                         ",\"bitDepth\":" + std::to_string(ch.fromBits) + "}";
      entry.afterJson = "{\"container\":\"" + ch.toContainer +
                        "\",\"sampleRate\":" + std::to_string(ch.toRate) +
                        ",\"bitDepth\":" + std::to_string(ch.toBits) +
                        ",\"durationMs\":" + std::to_string(ch.outDurationMs) + "}";
      entry.timestampUnix = static_cast<std::int64_t>(std::time(nullptr));
      audit.insert(entry);
      ++processed;
    } catch (...) {
      ++failed;
    }
  }
  emit progress(total, total, {});
  emit finished(processed, failed);
}

}  // namespace djapp
