find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(dj-lib-goodizer
  main.cpp
  MainWindow.cpp
  MainWindow.h
  AppDatabase.cpp
  AppDatabase.h
  LibraryModel.cpp
  LibraryModel.h
  ImportController.cpp
  ImportController.h
  ImportSession.cpp
  ImportSession.h
  WasmBridge.cpp  # body is #ifdef __EMSCRIPTEN__; compiles to nothing elsewhere
)

target_link_libraries(dj-lib-goodizer PRIVATE djcore Qt6::Widgets)

# Qt for WebAssembly emits <target>.html/.js/.wasm. Name the entry page
# index.html so the e2e harness and a plain static server can serve it as the
# directory default.
if(EMSCRIPTEN)
  set_target_properties(dj-lib-goodizer PROPERTIES
    OUTPUT_NAME "index"
    QT_WASM_INITIAL_MEMORY "67108864")
endif()

set_target_properties(dj-lib-goodizer PROPERTIES
  WIN32_EXECUTABLE ON
  MACOSX_BUNDLE ON
)

# Install rules drive both `cmake --install` (Linux tarball / AppDir) and the
# Windows windeployqt staging used by the packaging CI job and the scripts under
# packaging/.
include(GNUInstallDirs)
install(TARGETS dj-lib-goodizer
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  BUNDLE DESTINATION .
)
install(FILES ${CMAKE_SOURCE_DIR}/README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
