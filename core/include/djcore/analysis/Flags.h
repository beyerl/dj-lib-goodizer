#pragma once

#include "djcore/model/AnalysisResult.h"
#include "djcore/model/Enums.h"
#include "djcore/model/TargetProfile.h"

namespace djcore {

// Maps continuous metrics to the three-state flag model (spec §5.2): OK,
// Review (borderline), Outside Target. Used to colour the Library View and
// drive filtering. Stereo-only metrics return Ok when unset (mono tracks).
FlagState loudnessFlag(const AnalysisResult& r, const TargetProfile& p);
FlagState dynamicRangeFlag(const AnalysisResult& r, const TargetProfile& p);
FlagState monoFlag(const AnalysisResult& r, const TargetProfile& p);
FlagState widthFlag(const AnalysisResult& r, const TargetProfile& p);

const char* flagLabel(FlagState s);  // "OK" / "Review" / "Outside Target"

}  // namespace djcore
