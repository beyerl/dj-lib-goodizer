#include "djcore/audio/Dither.h"

#include <algorithm>
#include <cmath>

namespace djcore {

TpdfDither::TpdfDither(std::uint64_t seed) : state_(seed ? seed : 1ULL) {}

double TpdfDither::nextUniform() {
  // xorshift64* — fast, deterministic, good enough for dither noise.
  std::uint64_t x = state_;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  state_ = x;
  const std::uint64_t r = x * 0x2545F4914F6CDD1DULL;
  // Map the top 53 bits to [0, 1), then shift to [-0.5, 0.5).
  const double u = static_cast<double>(r >> 11) / 9007199254740992.0;
  return u - 0.5;
}

std::int32_t TpdfDither::quantize(float sample, int bits) {
  const double maxCode = static_cast<double>((1 << (bits - 1)) - 1);
  const double minCode = -static_cast<double>(1 << (bits - 1));

  // Triangular noise of ±1 LSB = sum of two independent uniform half-LSBs.
  const double lsb = 1.0;  // working in integer-code units below
  const double noise = (nextUniform() + nextUniform()) * lsb;

  const double scaled = static_cast<double>(sample) * (maxCode + 1.0);
  const double dithered = std::round(scaled + noise);
  return static_cast<std::int32_t>(std::clamp(dithered, minCode, maxCode));
}

}  // namespace djcore
