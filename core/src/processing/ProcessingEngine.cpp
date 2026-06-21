#include "djcore/processing/ProcessingEngine.h"

#include <algorithm>
#include <cmath>

#include "djcore/audio/AudioDecoder.h"
#include "djcore/audio/AudioEncoder.h"
#include "djcore/audio/Resampler.h"
#include "djcore/processing/Operations.h"

namespace djcore {
namespace {

constexpr double kTruePeakCeilingDb = -1.0;  // leave 1 dB of headroom

std::int64_t msToFrames(std::int64_t ms, int rate) {
  return ms * rate / 1000;
}

}  // namespace

ProcessingChange ProcessingEngine::plan(const Track& track,
                                        const AnalysisResult& analysis,
                                        const ProcessingOptions& options) const {
  ProcessingChange ch;
  ch.fromRate = track.format.sampleRate;
  ch.toRate = profile_.sampleRate;
  ch.resampled = track.format.sampleRate != profile_.sampleRate;
  ch.fromBits = track.format.bitDepth;
  ch.toBits = profile_.bitDepth;
  ch.requantized = track.format.bitDepth != profile_.bitDepth;
  ch.fromContainer = track.format.container;
  ch.toContainer = profile_.container;
  ch.containerChanged = track.format.container != profile_.container;

  if (options.applyTrim) {
    const std::int64_t lead =
        std::max<std::int64_t>(0, analysis.leadingSilenceMs - profile_.silence.leadInMs);
    if (lead > 0 || analysis.trailingSilenceMs > 0) {
      ch.trimmed = true;
      ch.trimLeadMs = lead;
      ch.trimTailMs = analysis.trailingSilenceMs;
    }
  }

  if (options.applyGain) {
    const double gainDb = profile_.loudnessTargetLufs - analysis.integratedLufs;
    if (std::abs(gainDb) > 0.05) {
      ch.gainApplied = true;
      ch.gainDb = gainDb;
      // Predict true-peak clipping and flag that gain would be limited.
      if (analysis.truePeakDbtp + gainDb > kTruePeakCeilingDb) {
        ch.gainLimitedForTruePeak = true;
        ch.gainDb = kTruePeakCeilingDb - analysis.truePeakDbtp;
      }
    }
  }

  ch.passthrough = !ch.resampled && !ch.requantized && !ch.containerChanged &&
                   !ch.trimmed && !ch.gainApplied;
  return ch;
}

ProcessingChange ProcessingEngine::process(const Track& track,
                                           const AnalysisResult& analysis,
                                           const std::string& outputPath,
                                           const ProcessingOptions& options) const {
  ProcessingChange ch = plan(track, analysis, options);

  auto decoder = openDecoder(track.sourcePath);
  PcmBuffer pcm = decoder->readAll();
  const int srcRate = pcm.sampleRate();

  // 1. Trim (standardizing).
  if (ch.trimmed) {
    const std::size_t startCut =
        static_cast<std::size_t>(msToFrames(ch.trimLeadMs, srcRate));
    const std::size_t endCut =
        static_cast<std::size_t>(msToFrames(ch.trimTailMs, srcRate));
    pcm = trimSilence(pcm, startCut, endCut);
  }

  // 2. Gain-only loudness normalization (standardizing; dynamics-preserving).
  if (ch.gainApplied) {
    GainOperation(std::pow(10.0, ch.gainDb / 20.0)).apply(pcm);
  }

  // 3. Resample to the target rate (standardizing).
  if (ch.resampled) {
    pcm = resample(pcm, profile_.sampleRate);
  }

  // 4. Encode to the output path with dither on bit-depth reduction.
  EncodeSpec spec;
  spec.container = profile_.container;
  spec.sampleRate = profile_.sampleRate;
  spec.bitDepth = profile_.bitDepth;
  spec.isFloat = false;
  encodeToFile(outputPath, pcm, spec);

  ch.outDurationMs = pcm.sampleRate() > 0
                         ? static_cast<std::int64_t>(pcm.frames()) * 1000 /
                               pcm.sampleRate()
                         : 0;
  return ch;
}

}  // namespace djcore
