#include "djcore/profile/DefaultProfiles.h"

namespace djcore {

std::vector<TargetProfile> defaultProfiles() {
  std::vector<TargetProfile> profiles;

  // Recommended first-run default for typical club / USB DJ-software use.
  TargetProfile club;
  club.name = "Club / USB";
  club.container = "flac";
  club.sampleRate = 44100;
  club.bitDepth = 16;
  club.loudnessTargetLufs = -14.0;
  club.drTolerance = {8.0, 2.0};
  club.monoSafetyThreshold = 0.2;
  club.widthTolerance = {0.5, 0.25};
  profiles.push_back(club);

  // Mono-safe broadcast playout: conservative mono threshold, narrower width.
  TargetProfile broadcast;
  broadcast.name = "Mono Broadcast";
  broadcast.container = "wav";
  broadcast.sampleRate = 48000;
  broadcast.bitDepth = 24;
  broadcast.loudnessTargetLufs = -16.0;
  broadcast.drTolerance = {6.0, 2.0};
  broadcast.monoSafetyThreshold = 0.5;
  broadcast.widthTolerance = {0.35, 0.2};
  profiles.push_back(broadcast);

  // Archival / lossless preservation: high rate/depth, wide tolerances.
  TargetProfile archival;
  archival.name = "Archival / Lossless";
  archival.container = "flac";
  archival.sampleRate = 96000;
  archival.bitDepth = 24;
  archival.loudnessTargetLufs = -18.0;
  archival.drTolerance = {12.0, 4.0};
  archival.monoSafetyThreshold = 0.0;
  archival.widthTolerance = {0.6, 0.4};
  profiles.push_back(archival);

  return profiles;
}

}  // namespace djcore
