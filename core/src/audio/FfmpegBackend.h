#pragma once

// Private declarations for the FFmpeg decode backend (compiled only when
// DJ_WITH_AUDIO_BACKENDS is on). Implemented in FfmpegDecoder.cpp and used by
// the openDecoder()/probeFormat()/canDecode() factory in AudioDecoder.cpp.

#include <memory>
#include <string>

#include "djcore/audio/AudioDecoder.h"
#include "djcore/audio/FormatInfo.h"

namespace djcore {

std::unique_ptr<AudioDecoder> openFfmpegDecoder(const std::string& path);
FormatInfo probeFfmpegFormat(const std::string& path);
bool ffmpegCanDecode(const std::string& path);

}  // namespace djcore
