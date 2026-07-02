# dj-lib-goodizer — Qt Widgets GUI over djcore. Built in CI (Qt is not installed
# on the dev machine). AUTOMOC/AUTOUIC are enabled here only, via
# qt_standard_project_setup().

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(dj-lib-goodizer
  main.cpp
  MainWindow.cpp
  MainWindow.h
  LibraryModel.cpp
  LibraryModel.h
  AppDatabase.cpp
  AppDatabase.h
  Worker.h
  WorkerSession.cpp
  WorkerSession.h
  ImportController.cpp
  ImportController.h
  ProcessController.cpp
  ProcessController.h
)

target_link_libraries(dj-lib-goodizer PRIVATE djcore Qt6::Widgets)

# GUI-subsystem app on Windows (no console window); --version still exits cleanly
# before QApplication for the CI startup smoke.
set_target_properties(dj-lib-goodizer PROPERTIES WIN32_EXECUTABLE ON)
