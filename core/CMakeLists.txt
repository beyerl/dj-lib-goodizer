# djcore — Qt-free domain library (headless-testable per the architecture plan).

# Vendored SQLite amalgamation (public domain, single .c file) — identical on
# every platform, no system package or vcpkg dependency required.
include(FetchContent)
FetchContent_Declare(
  sqlite_amalgamation
  URL https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(sqlite_amalgamation)
add_library(sqlite3 STATIC ${sqlite_amalgamation_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${sqlite_amalgamation_SOURCE_DIR})
if(NOT MSVC)
  target_compile_options(sqlite3 PRIVATE -w)  # silence amalgamation warnings
endif()

add_library(djcore
  src/Version.cpp
  src/audio/PcmBuffer.cpp
  src/audio/Dither.cpp
  src/audio/Resampler.cpp
  src/audio/WavIo.cpp
  src/analysis/AnalysisPipeline.cpp
  src/analysis/Flags.cpp
  src/analysis/analyzers/SilenceDetector.cpp
  src/analysis/analyzers/LoudnessDynamicsAnalyzer.cpp
  src/analysis/analyzers/PhaseMonoAnalyzer.cpp
  src/analysis/analyzers/StereoWidthAnalyzer.cpp
  src/db/Database.cpp
  src/db/Schema.cpp
  src/db/TrackRepository.cpp
  src/db/AnalysisResultRepository.cpp
  src/db/ProfileRepository.cpp
  src/db/AuditLogRepository.cpp
  src/jobs/AnalysisBatch.cpp
  src/processing/Operations.cpp
  src/processing/ProcessingEngine.cpp
  src/profile/DefaultProfiles.cpp
)

find_package(Threads REQUIRED)

target_include_directories(djcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(djcore PUBLIC cxx_std_20)
target_link_libraries(djcore PUBLIC sqlite3 Threads::Threads)

# Broad-format decode/encode (FFmpeg) + libsoxr/libebur128/TagLib are layered in
# under this flag. The dependency-free WAV backend above is always available.
if(DJ_WITH_AUDIO_BACKENDS)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavformat libavcodec libavutil libswresample)

  target_sources(djcore PRIVATE src/audio/FfmpegDecoder.cpp)
  target_link_libraries(djcore PUBLIC PkgConfig::FFMPEG)
  target_compile_definitions(djcore PUBLIC DJ_WITH_AUDIO_BACKENDS=1)
endif()
