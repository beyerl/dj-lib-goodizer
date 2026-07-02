#include <cstdio>
#include <cstring>

#include <QApplication>

#include "MainWindow.h"
#include "djcore/Version.h"

int main(int argc, char** argv) {
  // --version is handled before constructing QApplication so the CI startup
  // smoke works headlessly and exits fast (no display / GL needed).
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--version") == 0) {
      std::printf("dj-lib-goodizer %s\n", djcore::versionString());
      return 0;
    }
  }

  QApplication app(argc, argv);
  QApplication::setApplicationName("DJ Library Goodizer");
  QApplication::setApplicationVersion(djcore::versionString());
  QApplication::setOrganizationName("dj-lib-goodizer");

  djapp::MainWindow window;
  window.show();
  return app.exec();
}
