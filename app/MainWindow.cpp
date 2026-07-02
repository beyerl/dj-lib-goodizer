#include "MainWindow.h"

#include <map>

#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableView>
#include <QTextBrowser>
#include <QToolBar>

#include "ImportController.h"
#include "LibraryModel.h"
#include "ProcessController.h"
#include "WorkerSession.h"
#include "djcore/Version.h"
#include "djcore/analysis/Flags.h"
#include "djcore/db/AuditLogRepository.h"
#include "djcore/db/ProfileRepository.h"

namespace djapp {
namespace {

QString fmtOpt(const std::optional<double>& v, const char* unit, int prec = 2) {
  if (!v) return QStringLiteral("—");
  return QString::number(*v, 'f', prec) + QLatin1String(unit);
}

}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(
      QStringLiteral("DJ Library Goodizer %1").arg(djcore::versionString()));
  importSession_ = new WorkerSession(this);
  processSession_ = new WorkerSession(this);

  buildUi();
  buildMenus();

  connect(importSession_, &WorkerSession::progress, this, &MainWindow::onProgress);
  connect(importSession_, &WorkerSession::finished, this,
          &MainWindow::onImportFinished);
  connect(processSession_, &WorkerSession::progress, this, &MainWindow::onProgress);
  connect(processSession_, &WorkerSession::finished, this,
          &MainWindow::onProcessFinished);

  reloadProfiles();
  reloadLibrary();
  reloadDashboard();
  reloadAudit();
  reloadDetail();
  setBusy(false, {});
  resize(1100, 720);
}

void MainWindow::buildUi() {
  auto* toolbar = addToolBar(tr("Main"));
  toolbar->setMovable(false);
  toolbar->addWidget(new QLabel(tr("  Target profile: "), toolbar));
  profileCombo_ = new QComboBox(toolbar);
  toolbar->addWidget(profileCombo_);
  connect(profileCombo_, &QComboBox::currentIndexChanged, this,
          &MainWindow::onProfileChanged);

  tabs_ = new QTabWidget(this);
  setCentralWidget(tabs_);

  model_ = new LibraryModel(this);
  proxy_ = new QSortFilterProxyModel(this);
  proxy_->setSourceModel(model_);
  libraryView_ = new QTableView(this);
  libraryView_->setModel(proxy_);
  libraryView_->setSortingEnabled(true);
  libraryView_->setSelectionBehavior(QAbstractItemView::SelectRows);
  libraryView_->setSelectionMode(QAbstractItemView::SingleSelection);
  libraryView_->horizontalHeader()->setStretchLastSection(true);
  libraryView_->verticalHeader()->setVisible(false);
  connect(libraryView_->selectionModel(), &QItemSelectionModel::selectionChanged,
          this, &MainWindow::onSelectionChanged);
  tabs_->addTab(libraryView_, tr("Library"));

  detail_ = new QTextBrowser(this);
  tabs_->addTab(detail_, tr("Track Detail"));

  dashboard_ = new QTextBrowser(this);
  tabs_->addTab(dashboard_, tr("Dashboard"));

  audit_ = new QTextBrowser(this);
  tabs_->addTab(audit_, tr("Audit Log"));

  status_ = new QLabel(tr("Ready"), this);
  statusBar()->addWidget(status_, 1);
  progress_ = new QProgressBar(this);
  progress_->setMaximumWidth(240);
  statusBar()->addPermanentWidget(progress_);
  cancelButton_ = new QPushButton(tr("Cancel"), this);
  statusBar()->addPermanentWidget(cancelButton_);
  connect(cancelButton_, &QPushButton::clicked, this, &MainWindow::cancelWork);
}

void MainWindow::buildMenus() {
  auto* fileMenu = menuBar()->addMenu(tr("&File"));
  importAct_ = fileMenu->addAction(tr("&Import Folder…"), this,
                                   &MainWindow::importFolder);
  fileMenu->addAction(tr("&Export Audit Log…"), this, &MainWindow::exportAudit);
  fileMenu->addSeparator();
  fileMenu->addAction(tr("&Quit"), this, &QWidget::close);

  auto* processMenu = menuBar()->addMenu(tr("&Process"));
  processAct_ = processMenu->addAction(tr("&Standardize Library…"), this,
                                       &MainWindow::standardizeLibrary);

  auto* helpMenu = menuBar()->addMenu(tr("&Help"));
  helpMenu->addAction(tr("&About"), this, &MainWindow::showAbout);
}

bool MainWindow::isBusy() const {
  return importSession_->isRunning() || processSession_->isRunning();
}

djcore::TargetProfile MainWindow::activeProfile() const {
  const int i = profileCombo_->currentIndex();
  if (i >= 0 && i < static_cast<int>(profiles_.size()))
    return profiles_[static_cast<std::size_t>(i)];
  return {};
}

void MainWindow::reloadProfiles() {
  djcore::ProfileRepository repo(db_.db());
  profiles_ = repo.all();
  const QString previous = profileCombo_->currentText();
  profileCombo_->blockSignals(true);
  profileCombo_->clear();
  for (const auto& p : profiles_)
    profileCombo_->addItem(QString::fromStdString(p.name));
  int idx = profileCombo_->findText(previous);
  profileCombo_->setCurrentIndex(idx >= 0 ? idx : 0);
  profileCombo_->blockSignals(false);
  model_->setProfile(activeProfile());
}

void MainWindow::reloadLibrary() {
  model_->reload(db_.db());
  model_->setProfile(activeProfile());
  libraryView_->resizeColumnsToContents();
}

void MainWindow::reloadDetail() {
  const QModelIndex current = libraryView_->selectionModel()
                                  ? libraryView_->selectionModel()->currentIndex()
                                  : QModelIndex();
  if (!current.isValid()) {
    detail_->setHtml(tr("<p>Select a track in the Library to see its details.</p>"));
    return;
  }
  const int srcRow = proxy_->mapToSource(current).row();
  const LibraryRow* row = model_->rowAt(srcRow);
  if (!row) {
    detail_->clear();
    return;
  }
  const djcore::TargetProfile profile = activeProfile();
  const auto& t = row->track;

  QString html = QStringLiteral("<h2>%1</h2>")
                     .arg(QFileInfo(QString::fromStdString(t.sourcePath)).fileName());
  html += QStringLiteral("<p><b>Source:</b> %1</p>")
              .arg(QString::fromStdString(t.sourcePath));
  html += QStringLiteral(
              "<p><b>Format:</b> %1 / %2 · %3 Hz · %4-bit · %5 ch</p>")
              .arg(QString::fromStdString(t.format.container),
                   QString::fromStdString(t.format.codec))
              .arg(t.format.sampleRate)
              .arg(t.format.bitDepth)
              .arg(t.format.channels);

  if (!row->analysis) {
    html += tr("<p><i>Not analyzed.</i></p>");
    detail_->setHtml(html);
    return;
  }
  const auto& a = *row->analysis;
  html += QStringLiteral("<table cellpadding='4'>");
  auto addRow = [&](const QString& k, const QString& v, const QString& flag = {}) {
    html += QStringLiteral("<tr><td><b>%1</b></td><td>%2</td><td>%3</td></tr>")
                .arg(k, v, flag);
  };
  addRow(tr("Leading silence"), QString::number(a.leadingSilenceMs) + " ms");
  addRow(tr("Trailing silence"), QString::number(a.trailingSilenceMs) + " ms",
         a.silenceAmbiguous ? tr("ambiguous intro") : QString());
  addRow(tr("Integrated loudness"), QString::number(a.integratedLufs, 'f', 1) + " LUFS",
         QString::fromLatin1(djcore::flagLabel(djcore::loudnessFlag(a, profile))));
  addRow(tr("True peak"), QString::number(a.truePeakDbtp, 'f', 1) + " dBTP");
  addRow(tr("Crest factor"), QString::number(a.crestFactor, 'f', 2));
  addRow(tr("Dynamic range"), QString::number(a.drValue, 'f', 1) + " dB",
         QString::fromLatin1(djcore::flagLabel(djcore::dynamicRangeFlag(a, profile))));
  addRow(tr("Phase correlation"), fmtOpt(a.phaseCorrelation, ""),
         QString::fromLatin1(djcore::flagLabel(djcore::monoFlag(a, profile))));
  addRow(tr("Mono fold-down Δ"), fmtOpt(a.monoFolddownDeltaDb, " dB", 1));
  addRow(tr("Stereo width"), fmtOpt(a.stereoWidth, "", 2),
         QString::fromLatin1(djcore::flagLabel(djcore::widthFlag(a, profile))));
  addRow(tr("L/R balance"), fmtOpt(a.lrBalanceDb, " dB", 1));
  html += QStringLiteral("</table>");
  detail_->setHtml(html);
}

void MainWindow::reloadDashboard() {
  const auto& rows = model_->rows();
  const djcore::TargetProfile profile = activeProfile();
  int analyzed = 0, monoRisk = 0, widthOut = 0, loudnessOut = 0;
  double lufsSum = 0.0;
  std::map<std::string, int> formats;
  for (const auto& r : rows) {
    ++formats[r.track.format.container];
    if (!r.analysis) continue;
    ++analyzed;
    lufsSum += r.analysis->integratedLufs;
    if (djcore::monoFlag(*r.analysis, profile) != djcore::FlagState::Ok) ++monoRisk;
    if (djcore::widthFlag(*r.analysis, profile) != djcore::FlagState::Ok) ++widthOut;
    if (djcore::loudnessFlag(*r.analysis, profile) != djcore::FlagState::Ok)
      ++loudnessOut;
  }
  QString html = QStringLiteral("<h2>Library summary</h2>");
  html += QStringLiteral("<p><b>Tracks:</b> %1 &nbsp; <b>Analyzed:</b> %2</p>")
              .arg(rows.size())
              .arg(analyzed);
  if (analyzed > 0) {
    html += QStringLiteral("<p><b>Mean loudness:</b> %1 LUFS</p>")
                .arg(lufsSum / analyzed, 0, 'f', 1);
    html += QStringLiteral(
                "<p><b>Against profile \"%1\":</b> %2 loudness · %3 mono-risk · "
                "%4 width outliers</p>")
                .arg(QString::fromStdString(profile.name))
                .arg(loudnessOut)
                .arg(monoRisk)
                .arg(widthOut);
  }
  html += QStringLiteral("<h3>Formats</h3><ul>");
  for (const auto& [fmt, n] : formats)
    html += QStringLiteral("<li>%1: %2</li>").arg(QString::fromStdString(fmt)).arg(n);
  html += QStringLiteral("</ul>");
  dashboard_->setHtml(html);
}

void MainWindow::reloadAudit() {
  djcore::AuditLogRepository repo(db_.db());
  const auto entries = repo.all();
  QString html = QStringLiteral("<h2>Audit log (%1)</h2><table cellpadding='4'>")
                     .arg(entries.size());
  html += QStringLiteral(
      "<tr><th align='left'>#</th><th align='left'>Track</th>"
      "<th align='left'>Operation</th><th align='left'>After</th></tr>");
  for (const auto& e : entries) {
    html += QStringLiteral("<tr><td>%1</td><td>%2</td><td>%3</td><td>%4</td></tr>")
                .arg(e.id)
                .arg(e.trackId)
                .arg(QString::fromLatin1(djcore::toString(e.operation)),
                     QString::fromStdString(e.afterJson));
  }
  html += QStringLiteral("</table>");
  audit_->setHtml(html);
}

void MainWindow::setBusy(bool busy, const QString& what) {
  importAct_->setEnabled(!busy);
  processAct_->setEnabled(!busy);
  profileCombo_->setEnabled(!busy);
  progress_->setVisible(busy);
  cancelButton_->setVisible(busy);
  if (busy) {
    status_->setText(what);
  } else {
    status_->setText(tr("Ready"));
    progress_->reset();
  }
}

void MainWindow::importFolder() {
  if (isBusy()) return;
  const QString folder =
      QFileDialog::getExistingDirectory(this, tr("Select a music folder to import"));
  if (folder.isEmpty()) return;
  cancel_ = std::make_shared<std::atomic<bool>>(false);
  setBusy(true, tr("Importing & analyzing…"));
  importSession_->start(new ImportController(db_.path(), folder, cancel_));
}

void MainWindow::standardizeLibrary() {
  if (isBusy()) return;
  if (model_->trackCount() == 0) {
    QMessageBox::information(this, tr("Standardize"),
                             tr("Import a folder first."));
    return;
  }
  const QString outDir = QFileDialog::getExistingDirectory(
      this, tr("Select an output folder for the standardized library"));
  if (outDir.isEmpty()) return;
  lastOutDir_ = outDir;
  cancel_ = std::make_shared<std::atomic<bool>>(false);
  setBusy(true, tr("Standardizing…"));
  processSession_->start(new ProcessController(
      db_.path(), QString::fromStdString(activeProfile().name), outDir,
      /*applyGain=*/true, /*applyTrim=*/true, cancel_));
}

void MainWindow::exportAudit() {
  const QString path = QFileDialog::getSaveFileName(
      this, tr("Export audit log"), QStringLiteral("audit.json"),
      tr("JSON (*.json)"));
  if (path.isEmpty()) return;
  djcore::AuditLogRepository repo(db_.db());
  const auto entries = repo.all();
  QString out = QStringLiteral("[\n");
  for (int i = 0; i < static_cast<int>(entries.size()); ++i) {
    const auto& e = entries[static_cast<std::size_t>(i)];
    out += QStringLiteral(
               "  {\"id\":%1,\"trackId\":%2,\"operation\":\"%3\",\"after\":%4}%5\n")
               .arg(e.id)
               .arg(e.trackId)
               .arg(QString::fromLatin1(djcore::toString(e.operation)),
                    e.afterJson.empty() ? QStringLiteral("null")
                                        : QString::fromStdString(e.afterJson),
                    i + 1 < static_cast<int>(entries.size()) ? QStringLiteral(",")
                                                             : QString());
  }
  out += QStringLiteral("]\n");
  QFile f(path);
  if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
    f.write(out.toUtf8());
    f.close();
    statusBar()->showMessage(tr("Exported %1 audit entries").arg(entries.size()),
                             4000);
  }
}

void MainWindow::showAbout() {
  QMessageBox::about(
      this, tr("About DJ Library Goodizer"),
      tr("<h3>DJ Library Goodizer %1</h3>"
         "<p>Prepares DJ libraries: format/rate standardization, silence "
         "trimming, dynamic-range, mono-compatibility, and stereo-width "
         "analysis — non-destructive, with an audit log.</p>")
          .arg(djcore::versionString()));
}

void MainWindow::onProfileChanged(int) {
  model_->setProfile(activeProfile());
  reloadDetail();
  reloadDashboard();
}

void MainWindow::onSelectionChanged() { reloadDetail(); }

void MainWindow::onProgress(int done, int total, QString currentFile) {
  progress_->setMaximum(total > 0 ? total : 1);
  progress_->setValue(done);
  if (!currentFile.isEmpty())
    status_->setText(tr("%1 (%2/%3)").arg(currentFile).arg(done).arg(total));
}

void MainWindow::onImportFinished(int imported, int failed) {
  setBusy(false, {});
  reloadLibrary();
  reloadDashboard();
  reloadDetail();
  statusBar()->showMessage(
      tr("Imported %1 track(s), %2 failed").arg(imported).arg(failed), 5000);
}

void MainWindow::onProcessFinished(int processed, int failed) {
  setBusy(false, {});
  reloadLibrary();
  reloadAudit();
  reloadDashboard();
  QMessageBox::information(
      this, tr("Standardize complete"),
      tr("Processed %1 track(s), %2 failed.\nOutput written to:\n%3")
          .arg(processed)
          .arg(failed)
          .arg(lastOutDir_));
}

void MainWindow::cancelWork() {
  if (cancel_) cancel_->store(true);
  status_->setText(tr("Cancelling…"));
}

}  // namespace djapp
