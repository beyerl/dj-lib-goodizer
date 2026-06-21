#pragma once

#include <vector>

#include "djcore/model/TargetProfile.h"

namespace djcore {

// The named presets shipped with the application (FR-FMT-3, plan §6 / open Q1):
// "Club / USB", "Mono Broadcast", "Archival / Lossless". The first is the
// recommended first-run default.
std::vector<TargetProfile> defaultProfiles();

}  // namespace djcore
