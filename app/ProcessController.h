#pragma once

#include <atomic>
#include <memory>
#include <string>

#include <QString>

#include "Worker.h"

namespace djapp {

// Applies a target profile to every track on a worker thread, writing REAL
// conformed output files to `outDir` and recording an audit entry per track.
class ProcessController : public Worker {
  Q_OBJECT
 public:
  ProcessController(std::string dbPath, QString profileName, QString outDir,
                    bool applyGain, bool applyTrim,
                    std::shared_ptr<std::atomic<bool>> cancel,
                    QObject* parent = nullptr);

 public slots:
  void run() override;

 private:
  std::string dbPath_;
  QString profileName_;
  QString outDir_;
  bool applyGain_;
  bool applyTrim_;
  std::shared_ptr<std::atomic<bool>> cancel_;
};

}  // namespace djapp
