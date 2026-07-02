#include "djcore/processing/ProcessingEngine.h"

#include <utility>

#include "djcore/audio/AudioDecoder.h"
#include "djcore/audio/AudioEncoder.h"
#include "djcore/audio/PcmBuffer.h"

namespace djcore {

ProcessingEngine::ProcessingEngine(TargetProfile profile)
    : profile_(std::move(profile)) {}

ProcessingChange ProcessingEngine::plan(const Track& track,
                                        const AnalysisResult& /*analysis*/,
                                        const ProcessingOptions& /*options*/) const {
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

  // Trim (M5) and gain (M6) require analysis and are opt-in; not applied in M2.
  ch.outDurationMs = track.format.durationMs;
  ch.passthrough = !(ch.containerChanged || ch.resampled || ch.requantized ||
                     ch.trimmed || ch.gainApplied);
  return ch;
}

ProcessingChange ProcessingEngine::process(const Track& track,
                                           const AnalysisResult& analysis,
                                           const std::string& outputPath,
                                           const ProcessingOptions& options) const {
  ProcessingChange ch = plan(track, analysis, options);

  auto decoder = openDecoder(track.sourcePath);
  const FormatInfo& src = decoder->format();
  PcmBuffer buffer = decoder->readAll();

  // M2 conform = decode -> (re)encode to the target container/bit-depth.
  // Resample (M3), trim (M5), and gain (M6) are layered in later; the identity
  // "Archival / Lossless" profile keeps rate and depth, i.e. a faithful copy.
  bool floatOut = false;
  int outBits;
  if (profile_.bitDepth > 0) {
    outBits = profile_.bitDepth;
  } else {
    floatOut = (src.codec.rfind("pcm_f", 0) == 0);  // preserve a float source
    outBits = src.bitDepth > 0 ? src.bitDepth : 16;
  }

  EncodeSpec spec;
  spec.container = ch.toContainer;
  spec.bitDepth = outBits;
  spec.floatFormat = floatOut;
  encodeToFile(outputPath, buffer, spec);

  ch.toBits = floatOut ? 32 : outBits;
  ch.requantized = (ch.toBits != ch.fromBits);
  ch.outDurationMs = buffer.sampleRate()
                         ? static_cast<std::int64_t>(buffer.frames()) * 1000 /
                               buffer.sampleRate()
                         : 0;
  ch.passthrough = !(ch.containerChanged || ch.resampled || ch.requantized ||
                     ch.trimmed || ch.gainApplied);
  return ch;
}

}  // namespace djcore
