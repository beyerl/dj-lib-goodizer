#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <QString>

#include "Worker.h"

namespace djapp {

// Imports + analyzes a folder on a worker thread (opens its own WAL DB
// connection). Decodes each file once and runs the full analysis pipeline.
class ImportController : public Worker {
  Q_OBJECT
 public:
  ImportController(std::string dbPath, QString folder,
                   std::shared_ptr<std::atomic<bool>> cancel,
                   QObject* parent = nullptr);

 public slots:
  void run() override;

 private:
  std::string dbPath_;
  QString folder_;
  std::shared_ptr<std::atomic<bool>> cancel_;
};

}  // namespace djapp
