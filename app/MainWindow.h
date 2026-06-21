#pragma once

#include <QMainWindow>

class QTableView;

namespace djapp {

// Top-level window hosting the application's primary views (spec Table 10).
// Milestone 1 stands up the Library View shell and menu scaffolding; the
// detail/dashboard/profile/batch/audit views are layered in during M7.
class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);

 private slots:
  void onImportFolder();
  void onAbout();

 private:
  void buildMenus();

  QTableView* libraryView_ = nullptr;
};

}  // namespace djapp
