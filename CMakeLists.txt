cmake_minimum_required(VERSION 3.21)
project(dj-lib-goodizer VERSION 0.1.0 LANGUAGES CXX)

# ---- Global C++ settings -----------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# Qt's meta-object tooling (AUTOMOC/AUTOUIC/AUTORCC) is enabled only for the app
# target via qt_standard_project_setup(); the Qt-free core and tests don't need it.

# ---- Build options -----------------------------------------------------------
option(DJ_BUILD_APP   "Build the Qt GUI application" ON)
option(DJ_BUILD_TESTS "Build unit tests"             ON)

# Audio decode/encode backends (FFmpeg, libebur128, libsoxr, TagLib) land in
# Milestone 2. Keeping this OFF lets the Milestone-1 skeleton build everywhere
# without those heavy dependencies.
option(DJ_WITH_AUDIO_BACKENDS "Build with FFmpeg/libebur128/soxr/TagLib backends" OFF)

# ---- Targets -----------------------------------------------------------------
add_subdirectory(core)

if(DJ_BUILD_APP)
  add_subdirectory(app)
endif()

if(DJ_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
