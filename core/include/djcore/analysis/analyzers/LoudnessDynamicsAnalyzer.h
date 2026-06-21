#pragma once

#include <cstdint>
#include <vector>

#include "djcore/analysis/IAnalyzer.h"

namespace djcore {

// Computes integrated loudness (LUFS per ITU-R BS.1770 / EBU R128 — K-weighting
// + two-stage gating), sample/true-peak, crest factor, and a DR-value-style
// dynamic-range metric (FR-DR-1/2/8). Loudness and dynamic range are reported
// as separate metrics, never a single composite score (FR-DR-3).
//
// This is a dependency-free implementation using libebur128-compatible
// K-weighting coefficients; libebur128 itself can replace it behind
// DJ_WITH_AUDIO_BACKENDS for reference-grade validation.
class LoudnessDynamicsAnalyzer : public IAnalyzer {
 public:
  const char* name() const override { return "loudness"; }
  void begin(const FormatInfo& format) override;
  void processBlock(const PcmBlock& block) override;
  void finalize(AnalysisResult& result) override;

 private:
  // One stage of K-weighting, as a transposed direct-form-II biquad per channel.
  struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    void reset() { state.assign(state.size(), {0.0, 0.0}); }
    void setChannels(int n) { state.assign(n, {0.0, 0.0}); }
    double process(int ch, double x) {
      const double y = b0 * x + state[ch].s1;
      state[ch].s1 = b1 * x - a1 * y + state[ch].s2;
      state[ch].s2 = b2 * x - a2 * y;
      return y;
    }
    struct S {
      double s1, s2;
    };
    std::vector<S> state;
  };

  void designFilters(int sampleRate);

  int sampleRate_ = 0;
  int channels_ = 0;
  Biquad shelf_;
  Biquad hpf_;

  // 100 ms sub-blocks; an integration block is 4 sliding sub-blocks (400 ms,
  // 75% overlap) per BS.1770.
  std::int64_t subBlockSamples_ = 0;
  std::int64_t subFillCount_ = 0;
  double subSumSquares_ = 0.0;  // summed over channels for current sub-block
  std::vector<double> subMeanSquares_;  // completed sub-block mean-squares

  // Whole-track stats for peak / crest / DR.
  double peak_ = 0.0;
  double sumSquaresAll_ = 0.0;
  std::int64_t sampleCountAll_ = 0;

  // 3 s blocks for the DR-value metric.
  std::int64_t drBlockSamples_ = 0;
  std::int64_t drFillCount_ = 0;
  double drSumSquares_ = 0.0;
  std::vector<double> drBlockMeanSquares_;
};

}  // namespace djcore
