#pragma once

#include "djcore/audio/PcmBuffer.h"

namespace djcore {

// High-quality sample-rate conversion via a Kaiser-windowed-sinc kernel.
// `quality` is the number of zero-crossings on each side of the kernel (higher
// = sharper transition, more CPU). Anti-aliases to the destination Nyquist when
// downsampling. Returns the input unchanged (rate set) when already at dstRate.
PcmBuffer resample(const PcmBuffer& in, int dstSampleRate, int quality = 32);

}  // namespace djcore
