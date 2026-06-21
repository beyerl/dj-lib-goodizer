# djcore — Qt-free domain library (headless-testable per the architecture plan).
add_library(djcore
  src/Version.cpp
  src/audio/PcmBuffer.cpp
  src/audio/Dither.cpp
  src/audio/Resampler.cpp
  src/audio/WavIo.cpp
  src/profile/DefaultProfiles.cpp
)

target_include_directories(djcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(djcore PUBLIC cxx_std_20)

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
