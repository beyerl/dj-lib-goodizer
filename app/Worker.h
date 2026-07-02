#pragma once

#include <QObject>
#include <QString>

namespace djapp {

// Base class for a background task run on its own QThread via WorkerSession.
// Subclasses implement run() and emit progress()/finished(). Signals are queued
// to the GUI thread.
class Worker : public QObject {
  Q_OBJECT
 public:
  explicit Worker(QObject* parent = nullptr) : QObject(parent) {}

 public slots:
  virtual void run() = 0;

 signals:
  void progress(int done, int total, QString currentFile);
  void finished(int okCount, int failCount);
};

}  // namespace djapp
