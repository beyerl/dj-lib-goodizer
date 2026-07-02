#include "djcore/audio/AudioDecoder.h"

#include <algorithm>
#include <cctype>

#include "djcore/audio/AudioError.h"
#include "djcore/audio/WavIo.h"

#if DJ_WITH_AUDIO_BACKENDS
#include "FfmpegBackend.h"
#endif

namespace djcore {
namespace {

std::string lowerExt(const std::string& path) {
  const auto slash = path.find_last_of("/\\");
  const auto dot = path.find_last_of('.');
  if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
    return "";
  }
  std::string ext = path.substr(dot + 1);
  std::transform(ext.begin(), ext.end(), ext.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return ext;
}

// WAV backend (always available).
class WavDecoder : public AudioDecoder {
 public:
  explicit WavDecoder(std::string path) : path_(std::move(path)) {
    format_ = probeWav(path_);
  }
  const FormatInfo& format() const override { return format_; }
  PcmBuffer readAll() override { return readWav(path_); }

 private:
  std::string path_;
  FormatInfo format_;
};

}  // namespace

std::unique_ptr<AudioDecoder> openDecoder(const std::string& path) {
  const std::string ext = lowerExt(path);
  if (ext == "wav" || ext == "wave") {
    return std::make_unique<WavDecoder>(path);
  }
#if DJ_WITH_AUDIO_BACKENDS
  return openFfmpegDecoder(path);  // declared/implemented in FfmpegDecoder.cpp (M4)
#else
  throw AudioError("no decoder available for '" + path +
                   "' (rebuild with DJ_WITH_AUDIO_BACKENDS for broad formats)");
#endif
}

FormatInfo probeFormat(const std::string& path) {
  const std::string ext = lowerExt(path);
  if (ext == "wav" || ext == "wave") {
    return probeWav(path);
  }
#if DJ_WITH_AUDIO_BACKENDS
  return probeFfmpegFormat(path);  // M4
#else
  throw AudioError("cannot probe '" + path + "': no backend for this format");
#endif
}

bool canDecode(const std::string& path) {
  const std::string ext = lowerExt(path);
  if (ext == "wav" || ext == "wave") return true;
#if DJ_WITH_AUDIO_BACKENDS
  return ffmpegCanDecode(path);  // M4
#else
  return false;
#endif
}

}  // namespace djcore
