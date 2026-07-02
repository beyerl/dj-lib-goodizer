# djcli — Qt-free end-to-end harness. The command logic lives in a small static
# library (djcli_core) so the end-to-end test can drive the exact same code path
# the binary runs, in-process.

add_library(djcli_core STATIC CliCommands.cpp)
target_link_libraries(djcli_core PUBLIC djcore)
target_include_directories(djcli_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(djcli main.cpp)
target_link_libraries(djcli PRIVATE djcli_core)
