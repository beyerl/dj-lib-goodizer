# GoogleTest via FetchContent (no vcpkg dependency). Qt-free unit + end-to-end
# tests that run locally and in CI on both OSes.

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Keep the MSVC runtime consistent with our targets.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

# Core unit tests.
add_executable(djcore_tests
  smoke_test.cpp
  audio_test.cpp
  db_test.cpp
  analysis_test.cpp
  processing_test.cpp
  jobs_test.cpp
)
target_link_libraries(djcore_tests PRIVATE djcore GTest::gtest_main)
gtest_discover_tests(djcore_tests)

# End-to-end pipeline test (drives the same code the djcli binary runs).
if(TARGET djcli_core)
  add_executable(djcli_e2e_tests cli_e2e_test.cpp)
  target_link_libraries(djcli_e2e_tests PRIVATE djcli_core GTest::gtest_main)
  gtest_discover_tests(djcli_e2e_tests)
endif()

# Exercise the actual djcli binary in ctest (not just the library).
if(TARGET djcli)
  add_test(NAME djcli_binary_version COMMAND djcli --version)
endif()
