#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QStatusBar>
#include <QStringList>
#include <QTableView>

#include "djcore/Version.h"

namespace djapp {

namespace {

// Library View columns (spec Table 10): format, sample rate, loudness, dynamic
// range, mono-safety flag, stereo-width flag. Populated by the SQL-backed model
// in M4/M7; here they establish the working surface.
QStringList libraryColumns() {
  return {QObject::tr("Track"),       QObject::tr("Format"),
          QObject::tr("Sample Rate"), QObject::tr("Loudness (LUFS)"),
          QObject::tr("Dynamic Range"), QObject::tr("Mono"),
          QObject::tr("Width")};
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(tr("DJ Library Goodizer"));
  resize(1100, 700);

  libraryView_ = new QTableView(this);
  const QStringList columns = libraryColumns();
  auto* model = new QStandardItemModel(0, static_cast<int>(columns.size()), this);
  model->setHorizontalHeaderLabels(columns);
  libraryView_->setModel(model);
  libraryView_->setSortingEnabled(true);
  libraryView_->horizontalHeader()->setStretchLastSection(true);
  setCentralWidget(libraryView_);

  buildMenus();

  statusBar()->showMessage(
      tr("Ready — core v%1. Import a folder to begin.")
          .arg(QString::fromUtf8(djcore::coreVersionString())));
}

void MainWindow::buildMenus() {
  QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

  QAction* importAction = fileMenu->addAction(tr("&Import Folder…"));
  connect(importAction, &QAction::triggered, this, &MainWindow::onImportFolder);

  fileMenu->addSeparator();
  QAction* quitAction = fileMenu->addAction(tr("&Quit"));
  connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

  QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
  QAction* aboutAction = helpMenu->addAction(tr("&About"));
  connect(aboutAction, &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::onImportFolder() {
  const QString dir = QFileDialog::getExistingDirectory(
      this, tr("Select a library folder to import"));
  if (dir.isEmpty()) {
    return;
  }
  // Import/scan + analysis pipeline are wired in Milestones 3–5.
  statusBar()->showMessage(tr("Selected: %1 (import pipeline arrives in M3).").arg(dir));
}

void MainWindow::onAbout() {
  QMessageBox::about(
      this, tr("About DJ Library Goodizer"),
      tr("Desktop DJ Library Preparation Software\nCore v%1\n\n"
         "Standardizes, analyzes, and conditions audio libraries for "
         "consistent DJ playback.")
          .arg(QString::fromUtf8(djcore::coreVersionString())));
}

}  // namespace djapp
