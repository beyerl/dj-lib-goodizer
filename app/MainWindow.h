#pragma once

#include <QMainWindow>
#include <vector>

#include "AppDatabase.h"
#include "djcore/model/TargetProfile.h"

class QProgressBar;
class QSortFilterProxyModel;
class QTableView;
class QTextBrowser;
class QThread;

namespace djapp {

class LibraryModel;

// Primary window: Library / Dashboard / Audit tabs (spec Table 10). Import scans
// a folder, analyzes on a worker thread, and persists to the project DB; results
// drive the flag-coloured Library View and the dashboard summary.
class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override;

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
  QTextBrowser* detail_ = nullptr;
  QTextBrowser* dashboard_ = nullptr;
  QTextBrowser* audit_ = nullptr;
  QProgressBar* progress_ = nullptr;
  QThread* importThread_ = nullptr;
};

}  // namespace djapp
