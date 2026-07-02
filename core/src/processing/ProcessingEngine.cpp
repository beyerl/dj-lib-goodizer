#include "djcore/processing/ProcessingEngine.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <system_error>
#include <utility>

#include "djcore/audio/AudioDecoder.h"
#include "djcore/audio/AudioEncoder.h"
#include "djcore/audio/AudioError.h"
#include "djcore/audio/PcmBuffer.h"
#include "djcore/audio/Resampler.h"
#include "djcore/processing/Operations.h"

namespace djcore {
namespace {
constexpr double kTruePeakCeilingDbtp = -1.0;
}

ProcessingEngine::ProcessingEngine(TargetProfile profile)
    : profile_(std::move(profile)) {}

ProcessingChange ProcessingEngine::plan(const Track& track,
                                        const AnalysisResult& analysis,
                                        const ProcessingOptions& options) const {
  ProcessingChange ch;
  ch.fromContainer = track.format.container;
  ch.toContainer =
      profile_.container.empty() ? track.format.container : profile_.container;
  ch.containerChanged = (ch.toContainer != ch.fromContainer);

  ch.fromRate = track.format.sampleRate;
  ch.toRate = profile_.sampleRate > 0 ? profile_.sampleRate : track.format.sampleRate;
  ch.resampled = (ch.toRate != ch.fromRate);

  ch.fromBits = track.format.bitDepth;
  ch.toBits = profile_.bitDepth > 0 ? profile_.bitDepth : track.format.bitDepth;
  ch.requantized = (ch.toBits != ch.fromBits);

  // Silence trim (opt-in; never trims ambiguous quiet intros).
  if (options.applyTrim && !analysis.silenceAmbiguous) {
    std::int64_t lead = analysis.leadingSilenceMs - profile_.silence.leadInMs;
    if (lead < 0) lead = 0;
    const std::int64_t tail = analysis.trailingSilenceMs;
    if (lead > 0 || tail > 0) {
      ch.trimmed = true;
      ch.trimLeadMs = lead;
      ch.trimTailMs = tail;
    }
  }

  // Gain-only loudness normalization (opt-in; needs a target + a real measurement).
  if (options.applyGain && profile_.loudnessTargetLufs != 0.0 &&
      analysis.integratedLufs < 0.0) {
    double gainDb = profile_.loudnessTargetLufs - analysis.integratedLufs;
    gainDb = std::clamp(gainDb, -24.0, 24.0);
    if (analysis.truePeakDbtp + gainDb > kTruePeakCeilingDbtp) {
      gainDb = kTruePeakCeilingDbtp - analysis.truePeakDbtp;
      ch.gainLimitedForTruePeak = true;
    }
    if (std::fabs(gainDb) > 0.05) {
      ch.gainApplied = true;
      ch.gainDb = gainDb;
    }
  }

  std::int64_t dur = track.format.durationMs;
  if (ch.trimmed) dur -= (ch.trimLeadMs + ch.trimTailMs);
  ch.outDurationMs = dur > 0 ? dur : 0;

  ch.passthrough = !(ch.containerChanged || ch.resampled || ch.requantized ||
                     ch.trimmed || ch.gainApplied);
  return ch;
}

ProcessingChange ProcessingEngine::process(const Track& track,
                                           const AnalysisResult& analysis,
                                           const std::string& outputPath,
                                           const ProcessingOptions& options) const {
  ProcessingChange ch = plan(track, analysis, options);

  // Conform-skip: already conforming -> lossless copy (FR-FMT-4), no re-encode.
  if (ch.passthrough && ch.toContainer == track.format.container) {
    std::error_code ec;
    std::filesystem::copy_file(track.sourcePath, outputPath,
                               std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) throw AudioError("passthrough copy failed: " + ec.message());
    ch.outDurationMs = track.format.durationMs;
    return ch;
  }

  auto decoder = openDecoder(track.sourcePath);
  const FormatInfo src = decoder->format();
  PcmBuffer buffer = decoder->readAll();

  // Order: trim (source rate) -> resample -> gain -> encode (+dither on reduce).
  if (ch.trimmed) {
    auto framesFromMs = [&](std::int64_t ms) {
      return static_cast<std::size_t>(std::max<std::int64_t>(0, ms) * src.sampleRate /
                                      1000);
    };
    buffer = trimSilence(buffer, framesFromMs(ch.trimLeadMs), framesFromMs(ch.trimTailMs));
  }

  const int outRate = profile_.sampleRate > 0 ? profile_.sampleRate : src.sampleRate;
  if (outRate > 0 && outRate != src.sampleRate) {
    buffer = resample(buffer, outRate);
    ch.resampled = true;
    ch.toRate = outRate;
  }

  if (ch.gainApplied) {
    GainOperation(std::pow(10.0, ch.gainDb / 20.0)).apply(buffer);
  }

  bool floatOut = false;
  int outBits;
  if (profile_.bitDepth > 0) {
    outBits = profile_.bitDepth;
  } else {
    floatOut = (src.codec.rfind("pcm_f", 0) == 0);
    outBits = src.bitDepth > 0 ? src.bitDepth : 16;
  }

  EncodeSpec spec;
  spec.container = ch.toContainer;
  spec.bitDepth = outBits;
  spec.floatFormat = floatOut;
  spec.dither = (!floatOut && outBits < src.bitDepth);  // dither on reduction only
  encodeToFile(outputPath, buffer, spec);

  ch.toBits = floatOut ? 32 : outBits;
  ch.requantized = (ch.toBits != ch.fromBits);
  ch.outDurationMs = buffer.sampleRate()
                         ? static_cast<std::int64_t>(buffer.frames()) * 1000 /
                               buffer.sampleRate()
                         : 0;
  ch.passthrough = false;
  return ch;
}

}  // namespace djcore
