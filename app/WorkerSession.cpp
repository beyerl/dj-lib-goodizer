#include "WorkerSession.h"

#include <QThread>

#include "Worker.h"

namespace djapp {

WorkerSession::~WorkerSession() {
  // Teardown (not the event-loop slot path): blocking is fine here.
  if (thread_) {
    thread_->quit();
    thread_->wait();
  }
}

void WorkerSession::start(Worker* worker) {
  if (thread_) {
    worker->deleteLater();  // already busy
    return;
  }
  thread_ = new QThread(this);
  worker->moveToThread(thread_);

  connect(thread_, &QThread::started, worker, &Worker::run);
  connect(worker, &Worker::progress, this, &WorkerSession::progress);
  connect(worker, &Worker::finished, this, &WorkerSession::finished);
  connect(worker, &Worker::finished, thread_, &QThread::quit);
  connect(worker, &Worker::finished, worker, &QObject::deleteLater);
  connect(thread_, &QThread::finished, this, &WorkerSession::onThreadFinished);

  thread_->start();
}

void WorkerSession::onThreadFinished() {
  thread_->deleteLater();
  thread_ = nullptr;
}

}  // namespace djapp
