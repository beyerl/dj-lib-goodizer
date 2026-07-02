#pragma once

#include <optional>
#include <string>
#include <vector>

#include "djcore/model/TargetProfile.h"

namespace djcore {

// The named profiles shipped with the app. More are fleshed out in M3.
std::vector<TargetProfile> defaultProfiles();

// Looks up a shipped default profile by name (case-sensitive). std::nullopt if
// no such profile.
std::optional<TargetProfile> defaultProfileByName(const std::string& name);

}  // namespace djcore
