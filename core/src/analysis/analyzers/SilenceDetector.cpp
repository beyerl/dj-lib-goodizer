#include "djcore/analysis/analyzers/SilenceDetector.h"

#include <cmath>
#include <cstdlib>

namespace djcore {

void SilenceDetector::begin(const FormatInfo& format) {
  sampleRate_ = format.sampleRate;
  threshold_ = std::pow(10.0, cfg_.silenceThresholdDb / 20.0);
  ambiguous_ = std::pow(10.0, (cfg_.silenceThresholdDb + 20.0) / 20.0);
  firstLoud_ = -1;
  lastLoud_ = -1;
  totalFrames_ = 0;
  quietFrames_ = 0;
}

void SilenceDetector::processBlock(const PcmBlock& block) {
  for (std::size_t i = 0; i < block.frames; ++i) {
    float peak = 0.0f;
    for (int c = 0; c < block.channels; ++c) {
      float a = std::fabs(block.planes[c][i]);
      if (a > peak) peak = a;
    }
    const long long idx = totalFrames_ + static_cast<long long>(i);
    if (peak > threshold_) {
      if (firstLoud_ < 0) firstLoud_ = idx;
      lastLoud_ = idx;
      if (peak < ambiguous_) ++quietFrames_;
    }
  }
  totalFrames_ += static_cast<long long>(block.frames);
}

void SilenceDetector::finalize(AnalysisResult& result) {
  if (sampleRate_ <= 0) return;
  auto toMs = [&](long long frames) {
    return static_cast<std::int64_t>(frames * 1000 / sampleRate_);
  };

  if (firstLoud_ < 0) {  // entirely silent
    result.leadingSilenceMs = toMs(totalFrames_);
    result.trailingSilenceMs = 0;
    result.silenceAmbiguous = false;
    return;
  }

  const std::int64_t leadMs = toMs(firstLoud_);
  const std::int64_t trailMs = toMs(totalFrames_ - 1 - lastLoud_);
  const int minMs = cfg_.silenceMinDurationMs;
  // Min-duration gate: a run shorter than minDuration is not trimmable padding.
  result.leadingSilenceMs = leadMs >= minMs ? leadMs : 0;
  result.trailingSilenceMs = trailMs >= minMs ? trailMs : 0;
  // >= 250 ms of sustained quiet-but-present content -> ambiguous intro/outro.
  result.silenceAmbiguous = quietFrames_ > static_cast<long long>(sampleRate_) / 4;
}

}  // namespace djcore
