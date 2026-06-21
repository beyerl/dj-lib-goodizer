find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(dj-lib-goodizer
  main.cpp
  MainWindow.cpp
  MainWindow.h
)

target_link_libraries(dj-lib-goodizer PRIVATE djcore Qt6::Widgets)

set_target_properties(dj-lib-goodizer PROPERTIES
  WIN32_EXECUTABLE ON
  MACOSX_BUNDLE ON
)
