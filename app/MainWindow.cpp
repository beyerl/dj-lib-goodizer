#include "MainWindow.h"

#include <QLabel>

#include "djcore/Version.h"

namespace djapp {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(
      QStringLiteral("DJ Library Goodizer %1").arg(djcore::versionString()));

  auto* label = new QLabel(
      tr("DJ Library Goodizer\n\nImport a folder to begin. (UI arrives in a "
         "later milestone.)"),
      this);
  label->setAlignment(Qt::AlignCenter);
  label->setMargin(24);
  setCentralWidget(label);

  resize(960, 640);
}

}  // namespace djapp
