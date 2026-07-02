# djcli — Qt-free end-to-end harness. In later milestones this drives the full
# import -> analyze -> apply-profile -> write-real-files -> audit pipeline and is
# exercised by CI on every push. For M1 it parses --version/--help.

add_executable(djcli main.cpp)
target_link_libraries(djcli PRIVATE djcore)
