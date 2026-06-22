#pragma once

#include <QMainWindow>
#include <vector>

#include "AppDatabase.h"
#include "djcore/model/TargetProfile.h"

class QProgressBar;
class QSortFilterProxyModel;
class QTableView;
class QTabWidget;
class QTextBrowser;

namespace djapp {

class LibraryModel;
class ImportSession;

// Primary window: Library / Dashboard / Audit tabs (spec Table 10). Import scans
// a folder, analyzes on a worker thread, and persists to the project DB; results
// drive the flag-coloured Library View and the dashboard summary.
//
// In the WebAssembly build there is no native folder picker, worker threading,
// or on-disk audio, so the "Import Folder…" action is replaced by an in-memory
// "Load Demo Library" that seeds synthetic tracks. A JS test bridge
// (WasmBridge.cpp) drives this build for Playwright end-to-end tests; the public
// query/command methods below exist for that bridge and are harmless on desktop.
class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

  // --- Test/automation surface (used by the WASM JS bridge) ----------------
  // Seeds the library with synthetic, fully-analyzed tracks. No filesystem,
  // threads, or audio decode involved, so it is browser-safe and deterministic.
  void loadDemoLibrary();
  // Imports the given (virtual-)filesystem paths through the real
  // decode→analyze→persist pipeline. In the browser these are MEMFS paths the
  // folder-picker wrote the user's files to; only WAV decodes without the
  // FFmpeg backend (others are counted as failed). Synchronous (no worker
  // thread), so it is safe in the single-threaded WASM build.
  void importDiskFiles(const QStringList& paths);
  // Writes a short synthetic WAV to the virtual filesystem and imports it via
  // importDiskFiles — exercises the real disk-load path without a file picker
  // (used by the browser e2e tests).
  void loadDiskSample();
  int libraryRowCount() const;
  void selectRow(int sourceRow);
  void setCurrentTab(int index);
  int currentTab() const;
  void setProfileByIndex(int index);
  QString statusText() const;
  QString detailPlainText() const;
  QString dashboardPlainText() const;
  QString auditPlainText() const;
  QString selectedFileName() const;
  QString activeProfileName() const;
  double activeProfileTargetLufs() const;
  QStringList profileNames() const;
  QString aboutText() const;

 private slots:
  void onImportFolder();
  void onTrackSelected();
  void onImportProgress(int done, int total, const QString& current);
  void onImportFinished(int imported, int failed);
  void onAbout();

 private:
  void buildUi();
  void buildMenus();
  void loadProfiles();
  void setActiveProfile(int index);
  void reloadLibrary();
  void updateDashboard();

  AppDatabase db_;
  std::vector<djcore::TargetProfile> profiles_;
  djcore::TargetProfile activeProfile_;

  LibraryModel* model_ = nullptr;
  QSortFilterProxyModel* proxy_ = nullptr;
  QTableView* table_ = nullptr;
  QTabWidget* tabs_ = nullptr;
  QTextBrowser* detail_ = nullptr;
  QTextBrowser* dashboard_ = nullptr;
  QTextBrowser* audit_ = nullptr;
  QProgressBar* progress_ = nullptr;
  ImportSession* import_ = nullptr;
};

#ifdef __EMSCRIPTEN__
// Installs the JS test bridge (window.__djState / window.__djCmd). See
// WasmBridge.cpp. Called from main() in the WebAssembly build only.
void installWasmBridge(MainWindow* window);
#endif

}  // namespace djapp
