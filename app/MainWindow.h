#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <QMainWindow>
#include <QString>

#include "AppDatabase.h"
#include "djcore/model/TargetProfile.h"

class QAction;
class QComboBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QSortFilterProxyModel;
class QTabWidget;
class QTableView;
class QTextBrowser;

namespace djapp {

class LibraryModel;
class WorkerSession;

// The application window: Library / Track Detail / Dashboard / Audit tabs, a
// profile selector, and File/Process menus that drive background import+analyze
// and real profile-applying standardization.
class MainWindow : public QMainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QWidget* parent = nullptr);

 private slots:
  void importFolder();
  void standardizeLibrary();
  void exportAudit();
  void showAbout();
  void onProfileChanged(int index);
  void onSelectionChanged();
  void onProgress(int done, int total, QString currentFile);
  void onImportFinished(int imported, int failed);
  void onProcessFinished(int processed, int failed);
  void cancelWork();

 private:
  void buildUi();
  void buildMenus();
  void reloadProfiles();
  void reloadLibrary();
  void reloadDetail();
  void reloadDashboard();
  void reloadAudit();
  void setBusy(bool busy, const QString& what);
  bool isBusy() const;
  djcore::TargetProfile activeProfile() const;

  AppDatabase db_;
  LibraryModel* model_ = nullptr;
  QSortFilterProxyModel* proxy_ = nullptr;
  QTabWidget* tabs_ = nullptr;
  QTableView* libraryView_ = nullptr;
  QTextBrowser* detail_ = nullptr;
  QTextBrowser* dashboard_ = nullptr;
  QTextBrowser* audit_ = nullptr;
  QComboBox* profileCombo_ = nullptr;
  QProgressBar* progress_ = nullptr;
  QLabel* status_ = nullptr;
  QPushButton* cancelButton_ = nullptr;
  QAction* importAct_ = nullptr;
  QAction* processAct_ = nullptr;

  WorkerSession* importSession_ = nullptr;
  WorkerSession* processSession_ = nullptr;
  std::vector<djcore::TargetProfile> profiles_;
  std::shared_ptr<std::atomic<bool>> cancel_;
  QString lastOutDir_;
};

}  // namespace djapp
