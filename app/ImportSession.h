#pragma once

#include <QObject>
#include <QString>
#include <string>

class QThread;

namespace djapp {

// Owns the lifecycle of a background folder import: spins up a worker thread,
// forwards the worker's progress/finished signals, and tears the thread down
// safely once it has actually finished.
//
// Crucially, the thread is reaped via QThread::finished -> deleteLater on the
// GUI thread — never by calling QThread::wait() from inside the worker's
// `finished` slot. Doing the latter deadlocks: the finished signal also drives
// QThread::quit() as a *queued* call on the GUI thread, so a wait() in an
// earlier-connected finished slot blocks the GUI thread before quit() can run,
// and the thread never exits. (This was the "import progress bar freezes"
// bug.)
class ImportSession : public QObject {
  Q_OBJECT
 public:
  explicit ImportSession(QObject* parent = nullptr);
  ~ImportSession() override;

  // Begins importing `folder` into the database at `dbPath`. No-op if an
  // import is already running.
  void start(std::string dbPath, QString folder);
  bool isRunning() const { return thread_ != nullptr; }

 signals:
  void progress(int done, int total, QString currentFile);
  void finished(int imported, int failed);

 private slots:
  void onThreadFinished();

 private:
  QThread* thread_ = nullptr;
};

}  // namespace djapp
