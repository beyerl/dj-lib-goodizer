#pragma once

#include <QObject>
#include <QString>
#include <string>

namespace djapp {

// Runs folder import + analysis on a worker thread so the UI stays responsive
// (NFR-PERF-2). Opens its own SQLite (WAL) connection to the project DB. Emits
// queued signals the UI connects to for progress and completion.
class ImportController : public QObject {
  Q_OBJECT
 public:
  ImportController(std::string dbPath, QString folder, QObject* parent = nullptr);

 public slots:
  void run();

 signals:
  void progress(int done, int total, QString currentFile);
  void finished(int imported, int failed);

 private:
  std::string dbPath_;
  QString folder_;
};

}  // namespace djapp
