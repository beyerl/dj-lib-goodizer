#include "djcore/analysis/analyzers/LoudnessDynamicsAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace djcore {
namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kAbsoluteGateLufs = -70.0;
constexpr double kRelativeGateLu = -10.0;

double meanSquareToLufs(double meanSquare) {
  if (meanSquare <= 0.0) return -1000.0;
  return -0.691 + 10.0 * std::log10(meanSquare);
}
}  // namespace

void LoudnessDynamicsAnalyzer::designFilters(int fs) {
  const double f = static_cast<double>(fs);

  // Stage 1: high-shelf "pre-filter" (ITU-R BS.1770 / libebur128 constants).
  {
    const double f0 = 1681.974450955533;
    const double G = 3.999843853973347;
    const double Q = 0.7071752369554196;
    const double K = std::tan(kPi * f0 / f);
    const double Vh = std::pow(10.0, G / 20.0);
    const double Vb = std::pow(Vh, 0.4996667741545416);
    const double a0 = 1.0 + K / Q + K * K;
    shelf_.b0 = (Vh + Vb * K / Q + K * K) / a0;
    shelf_.b1 = 2.0 * (K * K - Vh) / a0;
    shelf_.b2 = (Vh - Vb * K / Q + K * K) / a0;
    shelf_.a1 = 2.0 * (K * K - 1.0) / a0;
    shelf_.a2 = (1.0 - K / Q + K * K) / a0;
  }
  // Stage 2: RLB high-pass.
  {
    const double f0 = 38.13547087602444;
    const double Q = 0.5003270373238773;
    const double K = std::tan(kPi * f0 / f);
    const double denom = 1.0 + K / Q + K * K;
    hpf_.b0 = 1.0;
    hpf_.b1 = -2.0;
    hpf_.b2 = 1.0;
    hpf_.a1 = 2.0 * (K * K - 1.0) / denom;
    hpf_.a2 = (1.0 - K / Q + K * K) / denom;
  }
}

void LoudnessDynamicsAnalyzer::begin(const FormatInfo& format) {
  sampleRate_ = format.sampleRate;
  channels_ = format.channels;
  designFilters(sampleRate_);
  shelf_.setChannels(channels_);
  hpf_.setChannels(channels_);

  subBlockSamples_ = static_cast<std::int64_t>(sampleRate_) / 10;     // 100 ms
  drBlockSamples_ = static_cast<std::int64_t>(sampleRate_) * 3;       // 3 s
  subFillCount_ = drFillCount_ = 0;
  subSumSquares_ = drSumSquares_ = 0.0;
  subMeanSquares_.clear();
  drBlockMeanSquares_.clear();
  peak_ = sumSquaresAll_ = 0.0;
  sampleCountAll_ = 0;
}

void LoudnessDynamicsAnalyzer::processBlock(const PcmBlock& block) {
  for (std::size_t i = 0; i < block.frames; ++i) {
    double kSqSum = 0.0;   // K-weighted sum of squares across channels
    double rawSqSum = 0.0;  // unweighted, for peak/crest/DR
    for (int ch = 0; ch < block.channels; ++ch) {
      const double x = static_cast<double>(block.planes[ch][i]);
      peak_ = std::max(peak_, std::abs(x));
      rawSqSum += x * x;
      const double k = hpf_.process(ch, shelf_.process(ch, x));
      kSqSum += k * k;
    }

    // K-weighted accumulation for loudness sub-blocks.
    subSumSquares_ += kSqSum;
    if (++subFillCount_ >= subBlockSamples_) {
      subMeanSquares_.push_back(subSumSquares_ /
                                static_cast<double>(subBlockSamples_));
      subSumSquares_ = 0.0;
      subFillCount_ = 0;
    }

    // Unweighted accumulation for peak/crest/DR.
    sumSquaresAll_ += rawSqSum;
    sampleCountAll_ += block.channels;
    drSumSquares_ += rawSqSum;
    if (++drFillCount_ >= drBlockSamples_) {
      drBlockMeanSquares_.push_back(
          drSumSquares_ / static_cast<double>(drBlockSamples_ * block.channels));
      drSumSquares_ = 0.0;
      drFillCount_ = 0;
    }
  }
}

void LoudnessDynamicsAnalyzer::finalize(AnalysisResult& result) {
  // ---- Integrated loudness (BS.1770 two-stage gating) -----------------------
  // Build 400 ms integration blocks from 4 sliding 100 ms sub-blocks.
  std::vector<double> blockMeanSquares;
  for (std::size_t i = 3; i < subMeanSquares_.size(); ++i) {
    const double ms = (subMeanSquares_[i] + subMeanSquares_[i - 1] +
                       subMeanSquares_[i - 2] + subMeanSquares_[i - 3]) /
                      4.0;
    blockMeanSquares.push_back(ms);
  }

  if (!blockMeanSquares.empty()) {
    // Absolute gate at -70 LUFS.
    double sumAbs = 0.0;
    int nAbs = 0;
    for (double ms : blockMeanSquares) {
      if (meanSquareToLufs(ms) >= kAbsoluteGateLufs) {
        sumAbs += ms;
        ++nAbs;
      }
    }
    if (nAbs > 0) {
      const double relThreshold =
          meanSquareToLufs(sumAbs / nAbs) + kRelativeGateLu;
      double sumRel = 0.0;
      int nRel = 0;
      for (double ms : blockMeanSquares) {
        if (meanSquareToLufs(ms) >= relThreshold) {
          sumRel += ms;
          ++nRel;
        }
      }
      result.integratedLufs =
          nRel > 0 ? meanSquareToLufs(sumRel / nRel) : meanSquareToLufs(sumAbs / nAbs);
    } else {
      result.integratedLufs = -70.0;
    }
  }

  // ---- Peak / crest ---------------------------------------------------------
  const double rms =
      sampleCountAll_ > 0
          ? std::sqrt(sumSquaresAll_ / static_cast<double>(sampleCountAll_))
          : 0.0;
  // Sample peak in dBFS. Full 4x-oversampled true-peak (dBTP) is provided by the
  // libebur128 backend; this is the sample-peak approximation.
  result.truePeakDbtp = peak_ > 0.0 ? 20.0 * std::log10(peak_) : -120.0;
  result.crestFactor = rms > 0.0 ? peak_ / rms : 0.0;

  // ---- DR value (TT DR-meter style) -----------------------------------------
  // RMS (quadratic mean) of the loudest 20% of 3 s blocks, referenced to peak.
  if (!drBlockMeanSquares_.empty()) {
    std::vector<double> ms = drBlockMeanSquares_;
    std::sort(ms.begin(), ms.end(), std::greater<double>());
    const std::size_t topN =
        std::max<std::size_t>(1, ms.size() / 5);  // top 20%
    double acc = 0.0;
    for (std::size_t i = 0; i < topN; ++i) acc += ms[i];
    const double upperRms = std::sqrt(acc / static_cast<double>(topN));
    result.drValue =
        (peak_ > 0.0 && upperRms > 0.0) ? 20.0 * std::log10(peak_ / upperRms) : 0.0;
  }
}

}  // namespace djcore
