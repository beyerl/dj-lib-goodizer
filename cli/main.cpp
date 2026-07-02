#include <cstdio>
#include <cstring>

#include "djcore/Version.h"

namespace {

void printUsage() {
  std::printf("djcli %s -- DJ library preparation CLI\n", djcore::versionString());
  std::printf("\n");
  std::printf("Usage: djcli [OPTIONS]\n");
  std::printf("\n");
  std::printf("Options:\n");
  std::printf("  -v, --version   Print version and exit\n");
  std::printf("  -h, --help      Print this help and exit\n");
  std::printf("\n");
  std::printf("Subcommands (import/analyze/apply/audit) land in later milestones.\n");
}

}  // namespace

int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0) {
      std::printf("djcli %s\n", djcore::versionString());
      return 0;
    }
    if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
      printUsage();
      return 0;
    }
  }

  std::printf("djcli %s\n", djcore::versionString());
  std::printf("No command given. Try --help.\n");
  return 0;
}
