#pragma once

#include <string>

#include "djcore/audio/FormatInfo.h"
#include "djcore/audio/PcmBuffer.h"

namespace djcore {

// Dependency-free RIFF/WAVE reader/writer. Reads PCM 16/24/32-bit integer and
// 32-bit IEEE float (incl. WAVE_FORMAT_EXTENSIBLE), converting to the canonical
// planar float32 PcmBuffer. Writes PCM 16/24-bit integer or 32-bit float.

// Reads a WAV file into a PcmBuffer. `infoOut` (if non-null) receives the
// source FormatInfo. Throws AudioError on failure.
PcmBuffer readWav(const std::string& path, FormatInfo* infoOut = nullptr);

// Reads only the WAV header/chunk metadata (no sample data) — cheap enough to
// run over every file during a folder import. Throws AudioError on failure.
FormatInfo probeWav(const std::string& path);

// Writes `buffer` as a WAV file at the given integer/float bit depth
// (16, 24, or 32-for-float). When `dither` is true and writing integer PCM,
// applies TPDF dither before quantization (use on bit-depth reduction).
// Throws AudioError on failure.
void writeWav(const std::string& path, const PcmBuffer& buffer, int bitDepth,
              bool floatFormat = false, bool dither = false);

}  // namespace djcore
