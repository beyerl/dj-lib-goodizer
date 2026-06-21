include(FetchContent)

# GoogleTest is fetched here (rather than via vcpkg) so the Milestone-1 skeleton
# builds and tests on any CI runner with no package-manager bootstrap.
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
)
# Honor the parent project's runtime library on Windows.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(djcore_tests
  smoke_test.cpp
  audio_test.cpp
  analysis_test.cpp
  db_test.cpp
  jobs_test.cpp
  processing_test.cpp
)
target_link_libraries(djcore_tests PRIVATE djcore GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(djcore_tests)
