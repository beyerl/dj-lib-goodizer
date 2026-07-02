#pragma once

#include <stdexcept>
#include <string>

namespace djcore {

// Thrown by decode/encode operations on failure (unreadable file, unsupported
// format, I/O error).
class AudioError : public std::runtime_error {
 public:
  explicit AudioError(const std::string& what) : std::runtime_error(what) {}
};

}  // namespace djcore
