#include "djcore/analysis/analyzers/LoudnessDynamicsAnalyzer.h"

#include <algorithm>
#include <cmath>

namespace djcore {
namespace {

constexpr double kPi = 3.14159265358979323846;

// Catmull-Rom cubic interpolation of four consecutive samples, evaluated at
// t in [0,1] between p1 and p2.
inline double cubic(double p0, double p1, double p2, double p3, double t) {
  const double a0 = -0.5 * p0 + 1.5 * p1 - 1.5 * p2 + 0.5 * p3;
  const double a1 = p0 - 2.5 * p1 + 2.0 * p2 - 0.5 * p3;
  const double a2 = -0.5 * p0 + 0.5 * p2;
  const double a3 = p1;
  return ((a0 * t + a1) * t + a2) * t + a3;
}

}  // namespace

void LoudnessDynamicsAnalyzer::begin(const FormatInfo& format) {
  sampleRate_ = format.sampleRate;
  channels_ = format.channels;
  const double fs = sampleRate_ > 0 ? sampleRate_ : 48000;

  // Stage 1: high-shelf pre-filter (BS.1770 K-weighting), libebur128 constants.
  {
    const double f0 = 1681.974450955533;
    const double G = 3.999843853973347;
    const double Q = 0.7071752369554196;
    const double K = std::tan(kPi * f0 / fs);
    const double Vh = std::pow(10.0, G / 20.0);
    const double Vb = std::pow(Vh, 0.4996667741545416);
    const double a0 = 1.0 + K / Q + K * K;
    Biquad b;
    b.b0 = (Vh + Vb * K / Q + K * K) / a0;
    b.b1 = 2.0 * (K * K - Vh) / a0;
    b.b2 = (Vh - Vb * K / Q + K * K) / a0;
    b.a1 = 2.0 * (K * K - 1.0) / a0;
    b.a2 = (1.0 - K / Q + K * K) / a0;
    stage1_.assign(channels_ > 0 ? channels_ : 1, b);
  }
  // Stage 2: RLB high-pass.
  {
    const double f0 = 38.13547087602444;
    const double Q = 0.5003270373238773;
    const double K = std::tan(kPi * f0 / fs);
    const double den = 1.0 + K / Q + K * K;
    Biquad b;
    b.b0 = 1.0;
    b.b1 = -2.0;
    b.b2 = 1.0;
    b.a1 = 2.0 * (K * K - 1.0) / den;
    b.a2 = (1.0 - K / Q + K * K) / den;
    stage2_.assign(channels_ > 0 ? channels_ : 1, b);
  }

  subLen_ = std::max<long>(1, static_cast<long>(fs / 10.0));  // 100 ms
  subPos_ = 0;
  subSum_ = 0.0;
  subBlockMs_.clear();

  peak_ = 0.0;
  sumSqAll_ = 0.0;
  sampleCountAll_ = 0;

  drLen_ = std::max<long>(1, static_cast<long>(fs * 3.0));  // 3 s
  drPos_ = 0;
  drSum_ = 0.0;
  drBlockMs_.clear();

  tp_.assign(channels_ > 0 ? channels_ : 1, TpWindow{});
  truePeak_ = 0.0;
}

void LoudnessDynamicsAnalyzer::processBlock(const PcmBlock& block) {
  const int ch = block.channels;
  for (std::size_t i = 0; i < block.frames; ++i) {
    double zFrame = 0.0;
    for (int c = 0; c < ch; ++c) {
      const float s = block.planes[c][i];
      const double a = std::fabs(static_cast<double>(s));
      if (a > peak_) peak_ = a;
      sumSqAll_ += static_cast<double>(s) * s;
      ++sampleCountAll_;
      drSum_ += static_cast<double>(s) * s;

      // K-weighting (channel weight 1.0 for L/R/C — surround weights omitted).
      const double k = stage2_[c].process(stage1_[c].process(static_cast<double>(s)));
      zFrame += k * k;

      // 4x true-peak via cubic interpolation between the two central samples.
      TpWindow& w = tp_[c];
      w.w0 = w.w1;
      w.w1 = w.w2;
      w.w2 = w.w3;
      w.w3 = s;
      ++w.count;
      if (w.count >= 4) {
        for (double t : {0.25, 0.5, 0.75}) {
          const double v = std::fabs(cubic(w.w0, w.w1, w.w2, w.w3, t));
          if (v > truePeak_) truePeak_ = v;
        }
      }
    }
    subSum_ += zFrame;
    if (++subPos_ >= subLen_) {
      subBlockMs_.push_back(subSum_ / static_cast<double>(subLen_));
      subSum_ = 0.0;
      subPos_ = 0;
    }
    if (++drPos_ >= drLen_) {
      const double denom = static_cast<double>(drLen_) * (ch > 0 ? ch : 1);
      drBlockMs_.push_back(drSum_ / denom);
      drSum_ = 0.0;
      drPos_ = 0;
    }
  }
}

void LoudnessDynamicsAnalyzer::finalize(AnalysisResult& result) {
  // Crest factor (linear peak/rms over all samples).
  const double rms =
      sampleCountAll_ > 0 ? std::sqrt(sumSqAll_ / static_cast<double>(sampleCountAll_))
                          : 0.0;
  result.crestFactor = rms > 0.0 ? peak_ / rms : 0.0;

  // True peak (dBTP), never below sample peak.
  const double tp = std::max(truePeak_, peak_);
  result.truePeakDbtp = tp > 0.0 ? 20.0 * std::log10(tp) : -120.0;

  // DR value: RMS of the loudest ~20% of 3 s blocks, referenced to peak.
  if (drBlockMs_.empty() && sampleCountAll_ > 0) {
    drBlockMs_.push_back(sumSqAll_ / static_cast<double>(sampleCountAll_));
  }
  if (!drBlockMs_.empty()) {
    std::sort(drBlockMs_.begin(), drBlockMs_.end(), std::greater<double>());
    const std::size_t topN = std::max<std::size_t>(1, drBlockMs_.size() / 5);
    double sum = 0.0;
    for (std::size_t i = 0; i < topN; ++i) sum += drBlockMs_[i];
    const double upperRms = std::sqrt(sum / static_cast<double>(topN));
    result.drValue =
        (peak_ > 0.0 && upperRms > 0.0) ? 20.0 * std::log10(peak_ / upperRms) : 0.0;
  }

  // Integrated loudness (EBU R128 two-stage gating).
  result.integratedLufs = -70.0;
  if (subBlockMs_.size() >= 4) {
    std::vector<double> blockMs;
    blockMs.reserve(subBlockMs_.size());
    for (std::size_t i = 3; i < subBlockMs_.size(); ++i) {
      blockMs.push_back(0.25 * (subBlockMs_[i] + subBlockMs_[i - 1] +
                                subBlockMs_[i - 2] + subBlockMs_[i - 3]));
    }
    // Absolute gate at -70 LUFS.
    double sumAbs = 0.0;
    int nAbs = 0;
    for (double m : blockMs) {
      if (m <= 0.0) continue;
      if (-0.691 + 10.0 * std::log10(m) >= -70.0) {
        sumAbs += m;
        ++nAbs;
      }
    }
    if (nAbs > 0) {
      const double lAbs = -0.691 + 10.0 * std::log10(sumAbs / nAbs);
      const double relThreshold = lAbs - 10.0;
      double sumRel = 0.0;
      int nRel = 0;
      for (double m : blockMs) {
        if (m <= 0.0) continue;
        const double l = -0.691 + 10.0 * std::log10(m);
        if (l >= -70.0 && l >= relThreshold) {
          sumRel += m;
          ++nRel;
        }
      }
      if (nRel > 0) result.integratedLufs = -0.691 + 10.0 * std::log10(sumRel / nRel);
    }
  }
}

}  // namespace djcore
