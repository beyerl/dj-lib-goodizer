include(FetchContent)

# GoogleTest is fetched here (rather than via vcpkg) so the Milestone-1 skeleton
# builds and tests on any CI runner with no package-manager bootstrap.
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
)
# Honor the parent project's runtime library on Windows.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# Keep GoogleTest out of `cmake --install` (packaging) — we only ship the app.
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(djcore_tests
  smoke_test.cpp
  audio_test.cpp
  analysis_test.cpp
  db_test.cpp
  jobs_test.cpp
  processing_test.cpp
  perceptual_test.cpp
)
target_link_libraries(djcore_tests PRIVATE djcore GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(djcore_tests)

# Qt-dependent tests for the app layer (background import lifecycle). Only built
# when the Qt app is in scope; reuses the app's ImportSession/ImportController
# sources and needs Qt's meta-object tooling (AUTOMOC) for their Q_OBJECTs.
if(DJ_BUILD_APP)
  find_package(Qt6 REQUIRED COMPONENTS Core)
  qt_standard_project_setup()

  # Bake the directories of linked shared libs (i.e. Qt) into the test's RPATH
  # so it can run from the build tree without LD_LIBRARY_PATH. Without this the
  # test loads fine on Windows (Qt DLLs are on PATH) but fails to start on Linux
  # (libQt6Core.so isn't on the loader path).
  set(CMAKE_BUILD_RPATH_USE_LINK_PATH ON)

  qt_add_executable(djapp_tests
    import_session_test.cpp
    ${CMAKE_SOURCE_DIR}/app/ImportSession.cpp
    ${CMAKE_SOURCE_DIR}/app/ImportSession.h
    ${CMAKE_SOURCE_DIR}/app/ImportController.cpp
    ${CMAKE_SOURCE_DIR}/app/ImportController.h
  )
  target_include_directories(djapp_tests PRIVATE ${CMAKE_SOURCE_DIR}/app)
  target_link_libraries(djapp_tests PRIVATE djcore Qt6::Core GTest::gtest_main)
  set_target_properties(djapp_tests PROPERTIES
    BUILD_RPATH "$<TARGET_FILE_DIR:Qt6::Core>")
  gtest_discover_tests(djapp_tests)
endif()
