# GoogleTest via FetchContent (no vcpkg dependency). djcore_tests holds the
# Qt-free unit + end-to-end tests that run locally and in CI on both OSes.

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Keep the MSVC runtime consistent with our targets.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(djcore_tests
  smoke_test.cpp
)
target_link_libraries(djcore_tests PRIVATE djcore GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(djcore_tests)
