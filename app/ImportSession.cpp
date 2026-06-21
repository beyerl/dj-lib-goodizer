#include "ImportSession.h"

#include <QThread>
#include <utility>

#include "ImportController.h"

namespace djapp {

ImportSession::ImportSession(QObject* parent) : QObject(parent) {}

ImportSession::~ImportSession() {
  // Teardown path (not the event-loop slot path): blocking here is fine because
  // we are being destroyed, not servicing the GUI event loop.
  if (thread_) {
    thread_->quit();
    thread_->wait();
  }
}

void ImportSession::start(std::string dbPath, QString folder) {
  if (thread_) return;  // an import is already running

  thread_ = new QThread(this);
  auto* worker = new ImportController(std::move(dbPath), std::move(folder));
  worker->moveToThread(thread_);

  connect(thread_, &QThread::started, worker, &ImportController::run);
  // Forward worker signals to our own (the UI connects to these).
  connect(worker, &ImportController::progress, this, &ImportSession::progress);
  connect(worker, &ImportController::finished, this, &ImportSession::finished);
  // When work completes: stop the worker thread's event loop and delete the
  // worker. quit() is a queued call on the GUI thread, which is why no slot may
  // block the GUI thread waiting on this thread.
  connect(worker, &ImportController::finished, thread_, &QThread::quit);
  connect(worker, &ImportController::finished, worker, &QObject::deleteLater);
  // Reap the thread only once it has genuinely finished — no GUI-thread wait().
  connect(thread_, &QThread::finished, this, &ImportSession::onThreadFinished);

  thread_->start();
}

void ImportSession::onThreadFinished() {
  thread_->deleteLater();
  thread_ = nullptr;
}

}  // namespace djapp
