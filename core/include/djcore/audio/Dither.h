#pragma once

#include <cstdint>

namespace djcore {

// Triangular-PDF (TPDF) dither for bit-depth reduction (FR-FMT-5). Adding ±1 LSB
// triangular noise before quantization decorrelates quantization error from the
// signal, trading a slightly higher noise floor for the elimination of audible
// quantization distortion.
//
// Deterministic for a given seed so tests are reproducible.
class TpdfDither {
 public:
  explicit TpdfDither(std::uint64_t seed = 0x9E3779B97F4A7C15ULL);

  // Quantize a float sample in [-1, 1] to a signed integer code of `bits`
  // resolution, applying TPDF dither. The result is clamped to the valid range.
  std::int32_t quantize(float sample, int bits);

 private:
  double nextUniform();  // uniform in [-0.5, 0.5)
  std::uint64_t state_;
};

}  // namespace djcore
