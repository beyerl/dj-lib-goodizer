#include "djcore/profile/DefaultProfiles.h"

namespace djcore {

std::vector<TargetProfile> defaultProfiles() {
  std::vector<TargetProfile> profiles;

  // Club / USB: the common club/DJ-software standard — 44.1 kHz / 16-bit WAV,
  // loud target, moderate dynamic-range and width bands.
  TargetProfile club;
  club.name = "Club / USB";
  club.container = "wav";
  club.sampleRate = 44100;
  club.bitDepth = 16;
  club.silence.thresholdDb = -60.0;
  club.silence.minDurationMs = 100;
  club.silence.leadInMs = 0;
  club.loudnessTargetLufs = -8.0;
  club.drTolerance = {8.0, 3.0};
  club.widthTolerance = {0.35, 0.2};
  club.monoSafetyThreshold = 0.2;
  profiles.push_back(club);

  // Mono Broadcast: stricter mono-safety and a quieter, more dynamic target.
  TargetProfile mono;
  mono.name = "Mono Broadcast";
  mono.container = "wav";
  mono.sampleRate = 44100;
  mono.bitDepth = 16;
  mono.silence.thresholdDb = -60.0;
  mono.silence.minDurationMs = 100;
  mono.silence.leadInMs = 0;
  mono.loudnessTargetLufs = -16.0;
  mono.drTolerance = {10.0, 4.0};
  mono.widthTolerance = {0.2, 0.15};
  mono.monoSafetyThreshold = 0.5;
  profiles.push_back(mono);

  // Archival / Lossless: keep source rate/depth; no loudness or perceptual
  // processing — a faithful, standardized copy. (FLAC support arrives with the
  // FFmpeg backend.)
  TargetProfile archival;
  archival.name = "Archival / Lossless";
  archival.container = "wav";
  archival.sampleRate = 0;  // keep source
  archival.bitDepth = 0;    // keep source
  archival.silence.thresholdDb = -60.0;
  archival.silence.minDurationMs = 100;
  archival.loudnessTargetLufs = 0.0;
  profiles.push_back(archival);

  return profiles;
}

std::optional<TargetProfile> defaultProfileByName(const std::string& name) {
  for (const auto& p : defaultProfiles()) {
    if (p.name == name) return p;
  }
  return std::nullopt;
}

}  // namespace djcore
