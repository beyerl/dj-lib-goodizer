# djcore — Qt-free domain library (headless-testable per the architecture plan).

include(FetchContent)

# Vendored SQLite amalgamation (public domain, single .c file) — identical on
# every platform, no system package or vcpkg dependency required. Pulled in at
# M1 so the FetchContent path is validated in CI before the db layer (M2) needs
# it.
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

# Generate djcore/Version.h from the project version.
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/Version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/djcore/Version.h
  @ONLY
)

add_library(djcore
  src/Version.cpp
  src/model/Enums.cpp
  src/audio/PcmBuffer.cpp
  src/audio/WavIo.cpp
  src/audio/AudioDecoder.cpp
  src/audio/AudioEncoder.cpp
  src/audio/Resampler.cpp
  src/audio/Dither.cpp
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
  src/db/AuditLogRepository.cpp
  src/db/ProfileRepository.cpp
  src/processing/Operations.cpp
  src/processing/ProcessingEngine.cpp
  src/profile/DefaultProfiles.cpp
  src/jobs/JobControl.cpp
  src/jobs/ParallelFor.cpp
  src/jobs/AnalysisBatch.cpp
)

target_include_directories(djcore PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_BINARY_DIR}/generated
)
target_compile_features(djcore PUBLIC cxx_std_20)

find_package(Threads REQUIRED)
target_link_libraries(djcore PUBLIC sqlite3 Threads::Threads)

# Broad-format decode/encode (FFmpeg) + libsoxr/libebur128/TagLib are layered in
# under this flag starting M4. The dependency-free WAV backend (M2) is always
# available.
if(DJ_WITH_AUDIO_BACKENDS)
  target_compile_definitions(djcore PUBLIC DJ_WITH_AUDIO_BACKENDS=1)
endif()
