#pragma once

#include <stdexcept>
#include <string>

namespace djcore {

// Thrown by the audio I/O layer on decode/encode failures.
class AudioError : public std::runtime_error {
 public:
  explicit AudioError(const std::string& what) : std::runtime_error(what) {}
};

}  // namespace djcore
