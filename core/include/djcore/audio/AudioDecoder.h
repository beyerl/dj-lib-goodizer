#pragma once

#include <memory>
#include <string>

#include "djcore/audio/FormatInfo.h"
#include "djcore/audio/PcmBuffer.h"

namespace djcore {

// Abstraction over a decode backend (plan §1). The canonical output is always
// float32 planar PCM (PcmBuffer / PcmBlock), regardless of the source codec, so
// analyzers and processors are backend-agnostic.
//
// Backends:
//   - WavDecoder      : dependency-free, default; covers the WAV test corpus.
//   - FfmpegDecoder   : broad-format (MP3/AAC/OGG/ALAC/FLAC/AIFF), built only
//                       when DJ_WITH_AUDIO_BACKENDS is enabled.
class AudioDecoder {
 public:
  virtual ~AudioDecoder() = default;

  // Probed container/codec metadata for the currently opened file.
  virtual const FormatInfo& format() const = 0;

  // Read up to maxFrames frames into the next block. Returns the number of
  // frames actually produced; 0 signals end-of-stream. The returned buffer is
  // owned by the decoder and valid until the next call.
  virtual std::size_t readBlock(PcmBuffer& out, std::size_t maxFrames) = 0;

  // Decode the entire file into a single buffer (convenience for short files
  // and tests). Streaming callers should prefer readBlock().
  virtual PcmBuffer readAll() = 0;
};

// Opens a file with the best available backend for its extension/content.
// Throws AudioError if no backend can handle it.
std::unique_ptr<AudioDecoder> openDecoder(const std::string& path);

}  // namespace djcore
