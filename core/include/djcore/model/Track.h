#pragma once

#include <cstdint>
#include <string>

#include "djcore/audio/FormatInfo.h"

namespace djcore {

// A source audio file in the working set. Non-destructive by default: the
// standardized copy is written to outputPath, leaving sourcePath untouched.
struct Track {
  std::int64_t id = 0;
  std::string sourcePath;
  std::string outputPath;  // empty until processed
  FormatInfo format;
  std::int64_t importedAtUnix = 0;
};

}  // namespace djcore
