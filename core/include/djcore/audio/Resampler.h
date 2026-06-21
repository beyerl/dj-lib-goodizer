#pragma once

#include "djcore/audio/PcmBuffer.h"

namespace djcore {

// High-quality sample-rate conversion (FR-FMT-4) via a band-limited
// windowed-sinc (Blackman-windowed) polyphase kernel, applied per channel.
//
// This dependency-free implementation is the default. libsoxr can be slotted in
// behind DJ_WITH_AUDIO_BACKENDS as an even higher-quality option without
// changing callers.
//
// On downsampling the kernel cutoff is lowered to the destination Nyquist to
// suppress aliasing. `quality` sets the half-width of the kernel in source-rate
// zero crossings (higher = sharper transition, more CPU).
PcmBuffer resample(const PcmBuffer& input, int dstSampleRate, int quality = 32);

}  // namespace djcore
