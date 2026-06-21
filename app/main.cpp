#include <QApplication>

#include "MainWindow.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QApplication::setApplicationName("DJ Library Goodizer");
  QApplication::setOrganizationName("dj-lib-goodizer");
  QApplication::setApplicationVersion("0.2.0");

  djapp::MainWindow window;
  window.show();

  return app.exec();
}
