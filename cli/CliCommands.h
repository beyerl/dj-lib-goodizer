#pragma once

namespace djcli {

// Parses argv and dispatches the djcli subcommand. Returns the process exit
// code. Exposed as a library entry point so the end-to-end test can drive the
// exact same code path the binary runs, in-process.
int runCli(int argc, char** argv);

}  // namespace djcli
