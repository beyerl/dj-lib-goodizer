#pragma once

#include <QMainWindow>

namespace djapp {

// Top-level window. For M1 this is an empty shell whose title carries the
// version; the six primary views are wired in Milestone 10.
class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = nullptr);
};

}  // namespace djapp
