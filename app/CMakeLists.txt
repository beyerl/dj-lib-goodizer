# dj-lib-goodizer — thin Qt Widgets GUI over djcore. Built in CI (Qt is not
# installed on the dev machine). AUTOMOC/AUTOUIC are enabled here only, via
# qt_standard_project_setup(); the Qt-free core/cli/tests do not need them.

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(dj-lib-goodizer
  main.cpp
  MainWindow.cpp
  MainWindow.h
)

target_link_libraries(dj-lib-goodizer PRIVATE djcore Qt6::Widgets)
