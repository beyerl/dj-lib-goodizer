#pragma once

#include "djcore/model/AnalysisResult.h"
#include "djcore/model/Enums.h"
#include "djcore/model/TargetProfile.h"

namespace djcore {

// Three-state flags mapping continuous metrics against a target profile. A
// zero/empty tolerance means "no target" and yields Ok. Stereo-only metrics
// read Ok when unset (mono tracks).
FlagState loudnessFlag(const AnalysisResult& r, const TargetProfile& p);
FlagState dynamicRangeFlag(const AnalysisResult& r, const TargetProfile& p);
FlagState monoFlag(const AnalysisResult& r, const TargetProfile& p);
FlagState widthFlag(const AnalysisResult& r, const TargetProfile& p);

// Plain-language label for a flag state.
const char* flagLabel(FlagState state);

}  // namespace djcore
