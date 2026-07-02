#include "djcore/profile/DefaultProfiles.h"

namespace djcore {

std::vector<TargetProfile> defaultProfiles() {
  std::vector<TargetProfile> profiles;

  // Archival / Lossless: keep the source sample rate and bit depth; container
  // WAV (FLAC support arrives with the audio backends in M4). No loudness or
  // perceptual processing — a faithful, standardized copy.
  TargetProfile archival;
  archival.name = "Archival / Lossless";
  archival.container = "wav";
  archival.sampleRate = 0;  // keep source
  archival.bitDepth = 0;    // keep source
  archival.loudnessTargetLufs = 0.0;
  archival.silence.thresholdDb = -60.0;
  archival.silence.minDurationMs = 100;
  profiles.push_back(archival);

  // "Club / USB" and "Mono Broadcast" are fleshed out in M3.
  return profiles;
}

std::optional<TargetProfile> defaultProfileByName(const std::string& name) {
  for (const auto& p : defaultProfiles()) {
    if (p.name == name) return p;
  }
  return std::nullopt;
}

}  // namespace djcore
