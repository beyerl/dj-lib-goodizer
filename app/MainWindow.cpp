#include "MainWindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QFileDialog>
#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStatusBar>
#include <QString>
#include <QTabWidget>
#include <QTableView>
#include <QTextBrowser>
#include <QThread>

#include "ImportController.h"
#include "LibraryModel.h"
#include "djcore/Version.h"
#include "djcore/analysis/Flags.h"
#include "djcore/db/ProfileRepository.h"
#include "djcore/db/TrackRepository.h"

namespace djapp {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(tr("DJ Library Goodizer"));
  resize(1180, 760);
  buildUi();
  buildMenus();
  loadProfiles();
  reloadLibrary();

  progress_ = new QProgressBar(this);
  progress_->setMaximumWidth(240);
  progress_->setVisible(false);
  statusBar()->addPermanentWidget(progress_);
  statusBar()->showMessage(
      tr("Ready — core v%1. Import a folder to analyze your library.")
          .arg(QString::fromUtf8(djcore::coreVersionString())));
}

MainWindow::~MainWindow() {
  if (importThread_) {
    importThread_->quit();
    importThread_->wait();
  }
}

void MainWindow::buildUi() {
  auto* tabs = new QTabWidget(this);

  // --- Library tab: table + detail panel ---
  model_ = new LibraryModel(this);
  proxy_ = new QSortFilterProxyModel(this);
  proxy_->setSourceModel(model_);
  proxy_->setSortRole(Qt::DisplayRole);

  table_ = new QTableView(this);
  table_->setModel(proxy_);
  table_->setSortingEnabled(true);
  table_->setSelectionBehavior(QAbstractItemView::SelectRows);
  table_->setSelectionMode(QAbstractItemView::SingleSelection);
  table_->horizontalHeader()->setStretchLastSection(true);
  connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &MainWindow::onTrackSelected);

  detail_ = new QTextBrowser(this);
  detail_->setMinimumWidth(320);
  detail_->setHtml(tr("<i>Select a track to see its metrics.</i>"));

  auto* split = new QSplitter(Qt::Horizontal, this);
  split->addWidget(table_);
  split->addWidget(detail_);
  split->setStretchFactor(0, 3);
  split->setStretchFactor(1, 1);
  tabs->addTab(split, tr("Library"));

  dashboard_ = new QTextBrowser(this);
  tabs->addTab(dashboard_, tr("Dashboard"));

  audit_ = new QTextBrowser(this);
  tabs->addTab(audit_, tr("Audit Log"));

  setCentralWidget(tabs);
}

void MainWindow::buildMenus() {
  QMenu* file = menuBar()->addMenu(tr("&File"));
  QAction* import = file->addAction(tr("&Import Folder…"));
  connect(import, &QAction::triggered, this, &MainWindow::onImportFolder);
  file->addSeparator();
  QAction* quit = file->addAction(tr("&Quit"));
  connect(quit, &QAction::triggered, qApp, &QApplication::quit);

  QMenu* help = menuBar()->addMenu(tr("&Help"));
  connect(help->addAction(tr("&About")), &QAction::triggered, this, &MainWindow::onAbout);
}

void MainWindow::loadProfiles() {
  djcore::ProfileRepository repo(db_.db());
  profiles_ = repo.all();
  if (profiles_.empty()) {
    profiles_.push_back(djcore::TargetProfile{});
  }
  activeProfile_ = profiles_.front();

  QMenu* menu = menuBar()->addMenu(tr("&Profile"));
  auto* group = new QActionGroup(this);
  group->setExclusive(true);
  for (int i = 0; i < static_cast<int>(profiles_.size()); ++i) {
    QAction* a = menu->addAction(QString::fromStdString(profiles_[i].name));
    a->setCheckable(true);
    a->setChecked(i == 0);
    group->addAction(a);
    connect(a, &QAction::triggered, this, [this, i] { setActiveProfile(i); });
  }
  model_->setProfile(activeProfile_);
}

void MainWindow::setActiveProfile(int index) {
  if (index < 0 || index >= static_cast<int>(profiles_.size())) return;
  activeProfile_ = profiles_[static_cast<std::size_t>(index)];
  model_->setProfile(activeProfile_);
  updateDashboard();
  statusBar()->showMessage(
      tr("Active profile: %1").arg(QString::fromStdString(activeProfile_.name)));
}

void MainWindow::reloadLibrary() {
  model_->reload(db_.db());
  table_->resizeColumnsToContents();
  updateDashboard();
}

void MainWindow::onImportFolder() {
  const QString dir = QFileDialog::getExistingDirectory(this, tr("Select a folder to import"));
  if (dir.isEmpty()) return;
  if (importThread_) return;  // an import is already running

  progress_->setVisible(true);
  progress_->setRange(0, 0);  // busy until first progress

  importThread_ = new QThread(this);
  auto* worker = new ImportController(db_.path(), dir);
  worker->moveToThread(importThread_);
  connect(importThread_, &QThread::started, worker, &ImportController::run);
  connect(worker, &ImportController::progress, this, &MainWindow::onImportProgress);
  connect(worker, &ImportController::finished, this, &MainWindow::onImportFinished);
  connect(worker, &ImportController::finished, importThread_, &QThread::quit);
  connect(worker, &ImportController::finished, worker, &QObject::deleteLater);
  importThread_->start();
}

void MainWindow::onImportProgress(int done, int total, const QString& current) {
  if (total > 0) {
    progress_->setRange(0, total);
    progress_->setValue(done);
  }
  if (!current.isEmpty()) {
    statusBar()->showMessage(tr("Analyzing %1 (%2/%3)").arg(current).arg(done).arg(total));
  }
}

void MainWindow::onImportFinished(int imported, int failed) {
  if (importThread_) {
    importThread_->wait();
    importThread_->deleteLater();
    importThread_ = nullptr;
  }
  progress_->setVisible(false);
  reloadLibrary();
  statusBar()->showMessage(tr("Imported %1 file(s), %2 failed.").arg(imported).arg(failed));
}

void MainWindow::onTrackSelected() {
  const QModelIndexList sel = table_->selectionModel()->selectedRows();
  if (sel.isEmpty()) return;
  const int srcRow = proxy_->mapToSource(sel.first()).row();
  const LibraryRow* row = model_->rowAt(srcRow);
  if (!row) return;

  const auto& fmt = row->track.format;
  QString html =
      QStringLiteral("<h3>%1</h3>")
          .arg(QString::fromStdString(row->track.sourcePath).toHtmlEscaped());
  html += QStringLiteral("<p><b>Format:</b> %1 / %2 — %3 Hz, %4-bit, %5 ch</p>")
              .arg(QString::fromStdString(fmt.container),
                   QString::fromStdString(fmt.codec))
              .arg(fmt.sampleRate)
              .arg(fmt.bitDepth)
              .arg(fmt.channels);

  if (row->analysis) {
    const auto& a = *row->analysis;
    html += QStringLiteral("<table cellpadding='4'>");
    const auto rowHtml = [&](const QString& k, const QString& v) {
      html += QStringLiteral("<tr><td><b>%1</b></td><td>%2</td></tr>").arg(k, v);
    };
    rowHtml(tr("Integrated loudness"), QString::number(a.integratedLufs, 'f', 1) + " LUFS");
    rowHtml(tr("True peak"), QString::number(a.truePeakDbtp, 'f', 1) + " dBFS");
    rowHtml(tr("Crest factor"), QString::number(a.crestFactor, 'f', 2));
    rowHtml(tr("DR value"), QString::number(a.drValue, 'f', 1) + " dB");
    rowHtml(tr("Leading silence"), QString::number(a.leadingSilenceMs) + " ms");
    rowHtml(tr("Trailing silence"), QString::number(a.trailingSilenceMs) + " ms");
    if (a.phaseCorrelation)
      rowHtml(tr("Phase correlation"), QString::number(*a.phaseCorrelation, 'f', 2));
    if (a.monoFolddownDeltaDb)
      rowHtml(tr("Mono fold-down"), QString::number(*a.monoFolddownDeltaDb, 'f', 1) + " dB");
    if (a.stereoWidth)
      rowHtml(tr("Stereo width"), QString::number(*a.stereoWidth, 'f', 2));
    if (a.lrBalanceDb)
      rowHtml(tr("L/R balance"), QString::number(*a.lrBalanceDb, 'f', 1) + " dB");
    html += QStringLiteral("</table>");
    html += QStringLiteral("<p><b>Mono:</b> %1 &nbsp; <b>Width:</b> %2</p>")
                .arg(djcore::flagLabel(djcore::monoFlag(a, activeProfile_)),
                     djcore::flagLabel(djcore::widthFlag(a, activeProfile_)));
  } else {
    html += QStringLiteral("<p><i>Not analyzed.</i></p>");
  }
  detail_->setHtml(html);
}

void MainWindow::updateDashboard() {
  const int n = model_->rowCount();
  if (n == 0) {
    dashboard_->setHtml(tr("<i>No tracks yet. Import a folder to populate the library.</i>"));
    audit_->setHtml(tr("<i>No processing has been performed yet.</i>"));
    return;
  }
  int analyzed = 0, monoRisk = 0;
  double lufsMin = 1e9, lufsMax = -1e9;
  for (int i = 0; i < n; ++i) {
    const LibraryRow* r = model_->rowAt(i);
    if (!r || !r->analysis) continue;
    ++analyzed;
    lufsMin = std::min(lufsMin, r->analysis->integratedLufs);
    lufsMax = std::max(lufsMax, r->analysis->integratedLufs);
    if (djcore::monoFlag(*r->analysis, activeProfile_) != djcore::FlagState::Ok) ++monoRisk;
  }
  QString html = QStringLiteral("<h3>Library Summary</h3>");
  html += QStringLiteral("<p><b>Tracks:</b> %1 (%2 analyzed)</p>").arg(n).arg(analyzed);
  if (analyzed > 0) {
    html += QStringLiteral("<p><b>Loudness range:</b> %1 to %2 LUFS</p>")
                .arg(QString::number(lufsMin, 'f', 1), QString::number(lufsMax, 'f', 1));
    html += QStringLiteral("<p><b>Mono-risk tracks:</b> %1</p>").arg(monoRisk);
    html += QStringLiteral("<p><b>Active profile:</b> %1 (target %2 LUFS)</p>")
                .arg(QString::fromStdString(activeProfile_.name))
                .arg(QString::number(activeProfile_.loudnessTargetLufs, 'f', 1));
  }
  dashboard_->setHtml(html);
}

void MainWindow::onAbout() {
  QMessageBox::about(
      this, tr("About DJ Library Goodizer"),
      tr("Desktop DJ Library Preparation Software\nCore v%1\n\n"
         "Standardizes, analyzes, and conditions audio libraries for consistent "
         "DJ playback.")
          .arg(QString::fromUtf8(djcore::coreVersionString())));
}

}  // namespace djapp
