#pragma once

#include <cstdint>

namespace djcore {

// Triangular-PDF (TPDF) dither generator with a deterministic PRNG so tests are
// reproducible. next() returns noise in [-1, 1) code-LSB units (sum of two
// independent uniforms), to be added before quantization on bit-depth reduction.
class TpdfDither {
 public:
  explicit TpdfDither(std::uint64_t seed = 0x9E3779B97F4A7C15ULL)
      : state_(seed ? seed : 1ULL) {}

  double next();

 private:
  double uniform();  // [-0.5, 0.5)
  std::uint64_t state_;
};

}  // namespace djcore
