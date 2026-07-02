cmake_minimum_required(VERSION 3.21)
project(dj-lib-goodizer VERSION 0.6.0 LANGUAGES C CXX)

# ---- Global C++ settings -----------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---- Build options -----------------------------------------------------------
# The Qt-free core + cli + tests build everywhere (including this dev machine,
# which has no Qt). The Qt GUI is built in CI. Broad-format audio backends
# (FFmpeg/soxr/TagLib) land in Milestone 4; keeping DJ_WITH_AUDIO_BACKENDS OFF
# lets the early milestones build with zero heavy external dependencies.
option(DJ_BUILD_APP           "Build the Qt GUI application"                 ON)
option(DJ_BUILD_CLI           "Build the headless djcli end-to-end tool"     ON)
option(DJ_BUILD_TESTS         "Build unit tests"                             ON)
option(DJ_WITH_AUDIO_BACKENDS "Build with FFmpeg/libsoxr/libebur128/TagLib"  OFF)

# ---- Targets -----------------------------------------------------------------
add_subdirectory(core)

if(DJ_BUILD_CLI)
  add_subdirectory(cli)
endif()

if(DJ_BUILD_APP)
  add_subdirectory(app)
endif()

if(DJ_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
