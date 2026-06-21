#pragma once

#include <cstdint>
#include <string>

#include "djcore/audio/FormatInfo.h"

namespace djcore {

// A source file in the working library, plus where its conditioned copy was
// written (empty until processed). Non-destructive by default: sourcePath is
// never overwritten unless in-place mode is explicitly enabled (NFR-SAFE-1).
struct Track {
  std::int64_t id = 0;
  std::string sourcePath;
  std::string outputPath;       // populated after processing
  FormatInfo format;
  std::int64_t importedAtUnix = 0;
};

}  // namespace djcore
