# djcore — Qt-free domain library (headless-testable per the architecture plan).
add_library(djcore
  src/Version.cpp
  src/audio/PcmBuffer.cpp
  src/profile/DefaultProfiles.cpp
)

target_include_directories(djcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(djcore PUBLIC cxx_std_20)

# The audio backends are layered in during Milestone 2 under this flag.
if(DJ_WITH_AUDIO_BACKENDS)
  target_compile_definitions(djcore PUBLIC DJ_WITH_AUDIO_BACKENDS=1)
endif()
