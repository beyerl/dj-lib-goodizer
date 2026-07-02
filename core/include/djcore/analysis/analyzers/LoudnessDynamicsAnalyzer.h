#pragma once

#include <cstdint>
#include <vector>

#include "djcore/analysis/IAnalyzer.h"

namespace djcore {

// Integrated loudness (ITU-R BS.1770-4 / EBU R128 K-weighting + two-stage
// gating), true peak (4x oversampled), crest factor, and DR value. All state is
// carried across processBlock calls so it works block-by-block.
class LoudnessDynamicsAnalyzer : public IAnalyzer {
 public:
  const char* name() const override { return "loudness"; }
  void begin(const FormatInfo& format) override;
  void processBlock(const PcmBlock& block) override;
  void finalize(AnalysisResult& result) override;

 private:
  // Transposed-Direct-Form-II biquad with double state, one per channel.
  struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    double z1 = 0, z2 = 0;
    double process(double x) {
      double y = b0 * x + z1;
      z1 = b1 * x - a1 * y + z2;
      z2 = b2 * x - a2 * y;
      return y;
    }
  };

  int sampleRate_ = 0;
  int channels_ = 0;

  std::vector<Biquad> stage1_;  // high-shelf, per channel
  std::vector<Biquad> stage2_;  // RLB high-pass, per channel

  // 100 ms sub-blocks of channel-weighted mean square.
  long subLen_ = 0;
  long subPos_ = 0;
  double subSum_ = 0.0;
  std::vector<double> subBlockMs_;

  // Crest factor: whole-track sample peak and mean square over all samples.
  double peak_ = 0.0;
  double sumSqAll_ = 0.0;
  long long sampleCountAll_ = 0;

  // DR value: 3 s block mean squares.
  long drLen_ = 0;
  long drPos_ = 0;
  double drSum_ = 0.0;
  std::vector<double> drBlockMs_;

  // True peak: 4x oversampling via cubic (Catmull-Rom) interpolation. Per
  // channel we keep a sliding 4-sample window and evaluate the inter-sample
  // maxima between the two central samples. Carried across blocks.
  struct TpWindow {
    float w0 = 0, w1 = 0, w2 = 0, w3 = 0;
    long long count = 0;
  };
  std::vector<TpWindow> tp_;
  double truePeak_ = 0.0;
};

}  // namespace djcore
