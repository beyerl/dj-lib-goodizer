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
#include <QFileInfo>
#include <QItemSelectionModel>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QTableView>
#include <QTextBrowser>

#include <cmath>
#include <ctime>
#include <filesystem>

#include "ImportSession.h"
#include "LibraryModel.h"
#include "djcore/Version.h"
#include "djcore/analysis/AnalysisPipeline.h"
#include "djcore/analysis/Flags.h"
#include "djcore/audio/AudioDecoder.h"
#include "djcore/audio/AudioEncoder.h"
#include "djcore/audio/PcmBuffer.h"
#include "djcore/db/AnalysisResultRepository.h"
#include "djcore/db/AuditLogRepository.h"
#include "djcore/db/ProfileRepository.h"
#include "djcore/db/TrackRepository.h"
#include "djcore/model/AnalysisResult.h"
#include "djcore/model/AuditLogEntry.h"
#include "djcore/model/Enums.h"
#include "djcore/model/Track.h"
#include "djcore/processing/ProcessingEngine.h"

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

  import_ = new ImportSession(this);
  connect(import_, &ImportSession::progress, this, &MainWindow::onImportProgress);
  connect(import_, &ImportSession::finished, this, &MainWindow::onImportFinished);

  statusBar()->showMessage(
      tr("Ready — core v%1. Import a folder to analyze your library.")
          .arg(QString::fromUtf8(djcore::coreVersionString())));
}

MainWindow::~MainWindow() = default;  // import_ (a child QObject) tears itself down

void MainWindow::buildUi() {
  auto* tabs = new QTabWidget(this);
  tabs_ = tabs;

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
#ifdef __EMSCRIPTEN__
  // No native folder dialog in the browser. Offer an in-memory demo library and
  // a folder picker (HTML file input) that imports real files from disk.
  QAction* demo = file->addAction(tr("&Load Demo Library"));
  connect(demo, &QAction::triggered, this, &MainWindow::loadDemoLibrary);
  QAction* disk = file->addAction(tr("Load &Folder from Disk…"));
  connect(disk, &QAction::triggered, this, [] { triggerFolderPicker(); });
#else
  QAction* import = file->addAction(tr("&Import Folder…"));
  connect(import, &QAction::triggered, this, &MainWindow::onImportFolder);
#endif
  file->addSeparator();
  QAction* quit = file->addAction(tr("&Quit"));
  connect(quit, &QAction::triggered, qApp, &QApplication::quit);

  QMenu* process = menuBar()->addMenu(tr("&Process"));
  QAction* standardize = process->addAction(tr("&Standardize Library (Dry Run)"));
  connect(standardize, &QAction::triggered, this, &MainWindow::standardizeLibrary);

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
  if (import_->isRunning()) return;  // an import is already running

  progress_->setVisible(true);
  progress_->setRange(0, 0);  // busy until first progress

  import_->start(db_.path(), dir);
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
  // No thread juggling here — ImportSession reaps its own thread. Blocking the
  // GUI thread in this slot is exactly what caused the import freeze.
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

QString MainWindow::aboutText() const {
  return tr("Desktop DJ Library Preparation Software\nCore v%1\n\n"
            "Standardizes, analyzes, and conditions audio libraries for "
            "consistent DJ playback.")
      .arg(QString::fromUtf8(djcore::coreVersionString()));
}

void MainWindow::onAbout() {
  QMessageBox::about(this, tr("About DJ Library Goodizer"), aboutText());
}

// --- Demo library (browser builds) ------------------------------------------

void MainWindow::loadDemoLibrary() {
  // A small, deterministic corpus that spans the flag states the UI colours:
  // in/around/outside the loudness + dynamics targets, plus mono-risk and
  // wide-stereo tracks. No filesystem, threads, or audio decode — safe in the
  // browser and reproducible for end-to-end tests.
  struct Demo {
    const char* file;
    const char* container;
    const char* codec;
    int rate;
    int bits;
    int channels;
    double lufs;
    double truePeak;
    double crest;
    double dr;
    long lead;
    long trail;
    bool stereo;
    double corr;
    double width;
    double balance;
  };
  static const Demo kDemo[] = {
      {"01 Opening Set.flac", "flac", "flac", 44100, 16, 2, -14.1, -1.2, 12.0,
       9.0, 120, 350, true, 0.62, 0.48, 0.3},
      {"02 Peak Time Banger.wav", "wav", "pcm_s16le", 44100, 16, 2, -7.8, -0.1,
       6.2, 5.0, 12, 40, true, 0.55, 0.41, -0.2},
      {"03 Deep Roller.aiff", "aiff", "pcm_s24be", 48000, 24, 2, -16.9, -2.4,
       14.5, 11.5, 540, 900, true, 0.71, 0.52, 0.1},
      {"04 Vocal Acappella.flac", "flac", "flac", 44100, 16, 1, -13.2, -1.0,
       10.1, 8.5, 30, 60, false, 0.0, 0.0, 0.0},
      {"05 Wide Synthwave.wav", "wav", "pcm_s24le", 44100, 24, 2, -12.7, -0.6,
       11.2, 9.5, 80, 200, true, -0.15, 0.93, 0.0},
      {"06 Lopsided Mix.mp3", "mp3", "mp3", 44100, 0, 2, -10.4, -0.3, 8.0, 6.5,
       8, 15, true, 0.48, 0.5, 4.6},
      {"07 Quiet Interlude.flac", "flac", "flac", 44100, 16, 2, -22.5, -6.0,
       16.0, 13.0, 1200, 1500, true, 0.80, 0.44, 0.0},
      {"08 Mono Risk Bootleg.mp3", "mp3", "mp3", 44100, 0, 2, -9.1, -0.2, 7.4,
       6.0, 5, 10, true, -0.62, 0.5, 0.4},
  };

  djcore::TrackRepository tracks(db_.db());
  djcore::AnalysisResultRepository results(db_.db());
  const auto now = static_cast<std::int64_t>(std::time(nullptr));

  for (const Demo& d : kDemo) {
    djcore::Track t;
    t.sourcePath = std::string("demo://") + d.file;
    t.format.container = d.container;
    t.format.codec = d.codec;
    t.format.sampleRate = d.rate;
    t.format.bitDepth = d.bits;
    t.format.channels = d.channels;
    t.format.sourceIsLossy = (std::string(d.codec) == "mp3");
    t.importedAtUnix = now;
    const std::int64_t id = tracks.insert(t);

    djcore::AnalysisResult r;
    r.leadingSilenceMs = d.lead;
    r.trailingSilenceMs = d.trail;
    r.integratedLufs = d.lufs;
    r.truePeakDbtp = d.truePeak;
    r.crestFactor = d.crest;
    r.drValue = d.dr;
    if (d.stereo) {
      r.phaseCorrelation = d.corr;
      r.monoFolddownDeltaDb = (d.corr < 0 ? -3.5 : -0.6);
      r.stereoWidth = d.width;
      r.lrBalanceDb = d.balance;
    }
    r.analyzerVersion = djcore::kAnalyzerVersion;
    r.analyzedAtUnix = now;
    results.upsert(id, r);
  }

  reloadLibrary();
  setCurrentTab(0);
  statusBar()->showMessage(
      tr("Loaded demo library — %1 track(s).").arg(libraryRowCount()));
}

void MainWindow::importDiskFiles(const QStringList& paths) {
  djcore::TrackRepository tracks(db_.db());
  djcore::AnalysisResultRepository results(db_.db());
  djcore::AnalysisPipeline pipeline;
  const auto now = static_cast<std::int64_t>(std::time(nullptr));

  int imported = 0;
  int failed = 0;
  for (const QString& qpath : paths) {
    const std::string path = qpath.toStdString();
    try {
      auto decoder = djcore::openDecoder(path);
      djcore::Track t;
      t.sourcePath = path;
      t.format = decoder->format();
      t.importedAtUnix = now;
      const std::int64_t id = tracks.insert(t);

      djcore::AnalysisResult r = pipeline.analyze(*decoder);
      r.analyzerVersion = djcore::kAnalyzerVersion;
      r.analyzedAtUnix = now;
      results.upsert(id, r);
      ++imported;
    } catch (...) {
      ++failed;  // e.g. non-WAV without the FFmpeg backend, or a bad file
    }
  }

  reloadLibrary();
  setCurrentTab(0);
  statusBar()->showMessage(
      tr("Imported %1 file(s) from disk, %2 failed.").arg(imported).arg(failed));
}

void MainWindow::loadDiskSample() {
  // Write a short, valid WAV to the (virtual) filesystem, then import it through
  // the real decode→analyze path — proving disk loading works end-to-end in the
  // browser without driving the native folder picker.
  constexpr double kPi = 3.14159265358979323846;
  const std::string dir = "/disk-sample";
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  const std::string path = dir + "/sample-tone.wav";

  djcore::PcmBuffer buf(2, 44100, 44100 / 2);  // 0.5 s stereo
  for (std::size_t i = 0; i < buf.frames(); ++i) {
    const double t = static_cast<double>(i) / 44100.0;
    const float v = static_cast<float>(0.3 * std::sin(2.0 * kPi * 440.0 * t));
    buf.channel(0)[i] = v;
    buf.channel(1)[i] = v;
  }
  djcore::encodeToFile(path, buf, djcore::EncodeSpec{"wav", 44100, 16, false});

  importDiskFiles({QString::fromStdString(path)});
}

void MainWindow::standardizeLibrary() {
  const int n = model_ ? model_->rowCount() : 0;
  if (n == 0) {
    statusBar()->showMessage(tr("Nothing to standardize — load a library first."));
    return;
  }

  djcore::ProcessingEngine engine(activeProfile_);
  djcore::AuditLogRepository auditRepo(db_.db());
  const auto now = static_cast<std::int64_t>(std::time(nullptr));

  QString html =
      QStringLiteral("<h3>Standardization plan (dry run) — profile: %1</h3>")
          .arg(QString::fromStdString(activeProfile_.name).toHtmlEscaped());
  html += QStringLiteral(
      "<table cellpadding='4'><tr><td><b>File</b></td>"
      "<td><b>Planned changes</b></td></tr>");

  int planned = 0, changed = 0, conform = 0;
  for (int i = 0; i < n; ++i) {
    const LibraryRow* r = model_->rowAt(i);
    if (!r || !r->analysis) continue;
    const djcore::ProcessingChange c = engine.plan(r->track, *r->analysis);
    ++planned;

    QStringList ops;
    if (c.containerChanged)
      ops << tr("container %1→%2").arg(QString::fromStdString(c.fromContainer),
                                       QString::fromStdString(c.toContainer));
    if (c.resampled)
      ops << tr("resample %1→%2 Hz").arg(c.fromRate).arg(c.toRate);
    if (c.requantized)
      ops << tr("requantize %1→%2-bit").arg(c.fromBits).arg(c.toBits);
    if (c.trimmed)
      ops << tr("trim %1/%2 ms").arg(c.trimLeadMs).arg(c.trimTailMs);
    if (c.gainApplied)
      ops << tr("gain %1 dB%2")
                 .arg(QString::number(c.gainDb, 'f', 1),
                      c.gainLimitedForTruePeak ? tr(" (TP-limited)") : QString());
    if (ops.isEmpty()) {
      ops << tr("already conforms");
      ++conform;
    } else {
      ++changed;
    }

    const QString file =
        QFileInfo(QString::fromStdString(r->track.sourcePath)).fileName();
    html += QStringLiteral("<tr><td>%1</td><td>%2</td></tr>")
                .arg(file.toHtmlEscaped(), ops.join(", "));

    djcore::AuditLogEntry e;
    e.trackId = r->track.id;
    e.operation = djcore::OperationType::ResampleTranscode;
    e.paramsJson = ops.join("; ").toStdString();
    e.timestampUnix = now;
    auditRepo.insert(e);
  }
  html += QStringLiteral("</table>");
  html += QStringLiteral(
              "<p><b>%1</b> track(s) planned — <b>%2</b> need changes, "
              "<b>%3</b> already conform. (Dry run: no files written.)</p>")
              .arg(planned)
              .arg(changed)
              .arg(conform);

  audit_->setHtml(html);
  setCurrentTab(2);
  statusBar()->showMessage(
      tr("Standardized %1 track(s) (dry run) — see Audit Log.").arg(planned));
}

// --- Test/automation accessors ----------------------------------------------

int MainWindow::libraryRowCount() const { return model_ ? model_->rowCount() : 0; }

void MainWindow::selectRow(int sourceRow) {
  if (!model_ || sourceRow < 0 || sourceRow >= model_->rowCount()) return;
  const QModelIndex src = model_->index(sourceRow, 0);
  const QModelIndex view = proxy_->mapFromSource(src);
  table_->selectionModel()->select(
      view, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  table_->setCurrentIndex(view);
}

void MainWindow::setCurrentTab(int index) {
  if (tabs_ && index >= 0 && index < tabs_->count()) tabs_->setCurrentIndex(index);
}

int MainWindow::currentTab() const { return tabs_ ? tabs_->currentIndex() : -1; }

void MainWindow::setProfileByIndex(int index) { setActiveProfile(index); }

QString MainWindow::statusText() const { return statusBar()->currentMessage(); }

QString MainWindow::detailPlainText() const {
  return detail_ ? detail_->toPlainText() : QString();
}

QString MainWindow::dashboardPlainText() const {
  return dashboard_ ? dashboard_->toPlainText() : QString();
}

QString MainWindow::auditPlainText() const {
  return audit_ ? audit_->toPlainText() : QString();
}

QString MainWindow::selectedFileName() const {
  if (!table_ || !table_->selectionModel()) return {};
  const QModelIndexList sel = table_->selectionModel()->selectedRows();
  if (sel.isEmpty()) return {};
  const int srcRow = proxy_->mapToSource(sel.first()).row();
  const LibraryRow* row = model_->rowAt(srcRow);
  if (!row) return {};
  return QFileInfo(QString::fromStdString(row->track.sourcePath)).fileName();
}

QString MainWindow::activeProfileName() const {
  return QString::fromStdString(activeProfile_.name);
}

double MainWindow::activeProfileTargetLufs() const {
  return activeProfile_.loudnessTargetLufs;
}

QStringList MainWindow::profileNames() const {
  QStringList names;
  for (const auto& p : profiles_) names << QString::fromStdString(p.name);
  return names;
}

}  // namespace djapp
