#include "djcore/audio/Dither.h"

namespace djcore {

double TpdfDither::uniform() {
  // xorshift64* PRNG.
  std::uint64_t x = state_;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  state_ = x;
  const std::uint64_t r = x * 0x2545F4914F6CDD1DULL;
  // Top 53 bits -> [0,1), then shift to [-0.5, 0.5).
  return static_cast<double>(r >> 11) / 9007199254740992.0 - 0.5;
}

double TpdfDither::next() { return uniform() + uniform(); }

}  // namespace djcore
