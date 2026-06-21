#include "djcore/analysis/analyzers/SilenceDetector.h"

#include <algorithm>
#include <cmath>

namespace djcore {

SilenceDetector::SilenceDetector(const AnalysisConfig& cfg)
    : thresholdLinear_(std::pow(10.0, cfg.silenceThresholdDb / 20.0)),
      ambiguousLinear_(std::pow(10.0, (cfg.silenceThresholdDb + 20.0) / 20.0)) {}

void SilenceDetector::begin(const FormatInfo& format) {
  sampleRate_ = format.sampleRate;
  totalFrames_ = 0;
  firstLoud_ = -1;
  lastLoud_ = -1;
  quietContentFrames_ = 0;
}

void SilenceDetector::processBlock(const PcmBlock& block) {
  for (std::size_t i = 0; i < block.frames; ++i) {
    double peak = 0.0;
    for (int ch = 0; ch < block.channels; ++ch) {
      peak = std::max(peak, std::abs(static_cast<double>(block.planes[ch][i])));
    }
    const std::int64_t idx = totalFrames_ + static_cast<std::int64_t>(i);
    if (peak > thresholdLinear_) {
      if (firstLoud_ < 0) firstLoud_ = idx;
      lastLoud_ = idx;
      if (peak < ambiguousLinear_) ++quietContentFrames_;
    }
  }
  totalFrames_ += static_cast<std::int64_t>(block.frames);
}

void SilenceDetector::finalize(AnalysisResult& result) {
  if (sampleRate_ <= 0 || totalFrames_ == 0) {
    return;
  }
  const auto framesToMs = [&](std::int64_t f) {
    return f * 1000 / sampleRate_;
  };

  if (firstLoud_ < 0) {
    // Entirely silent.
    result.leadingSilenceMs = framesToMs(totalFrames_);
    result.trailingSilenceMs = 0;
    return;
  }

  result.leadingSilenceMs = framesToMs(firstLoud_);
  result.trailingSilenceMs = framesToMs(totalFrames_ - 1 - lastLoud_);

  // Ambiguous when there is sustained low-level content (e.g. a slow fade-in)
  // rather than true digital silence before/after the main programme.
  const std::int64_t ambiguousMinFrames =
      static_cast<std::int64_t>(sampleRate_) / 4;  // 250 ms
  result.silenceAmbiguous = quietContentFrames_ > ambiguousMinFrames;
}

}  // namespace djcore
