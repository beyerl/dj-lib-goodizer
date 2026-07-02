#pragma once

#include <QObject>
#include <QString>

class QThread;

namespace djapp {

class Worker;

// Owns the lifecycle of a background Worker: moves it to a fresh QThread,
// forwards its progress/finished signals, and reaps the thread via
// QThread::finished -> deleteLater on the GUI thread. It NEVER calls
// QThread::wait() from a slot on the GUI thread — doing so deadlocks against the
// queued quit() (the historical "import progress bar freezes" bug).
class WorkerSession : public QObject {
  Q_OBJECT
 public:
  explicit WorkerSession(QObject* parent = nullptr) : QObject(parent) {}
  ~WorkerSession() override;

  // Takes ownership of `worker` and starts it. If a task is already running the
  // worker is discarded and the call is a no-op.
  void start(Worker* worker);
  bool isRunning() const { return thread_ != nullptr; }

 signals:
  void progress(int done, int total, QString currentFile);
  void finished(int okCount, int failCount);

 private slots:
  void onThreadFinished();

 private:
  QThread* thread_ = nullptr;
};

}  // namespace djapp
