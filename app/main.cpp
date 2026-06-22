#include <QApplication>

#include "MainWindow.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QApplication::setApplicationName("DJ Library Goodizer");
  QApplication::setOrganizationName("dj-lib-goodizer");
  QApplication::setApplicationVersion("0.13.1");

  djapp::MainWindow window;
  window.show();

#ifdef __EMSCRIPTEN__
  // Expose the JS test bridge for browser end-to-end tests.
  djapp::installWasmBridge(&window);
#endif

  return app.exec();
}
