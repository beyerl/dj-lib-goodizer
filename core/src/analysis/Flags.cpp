#include "djcore/analysis/Flags.h"

#include <cmath>

namespace djcore {
namespace {
// Generic band classifier: within `tol` is Ok, within 2*tol is Review.
FlagState classifyDelta(double delta, double tol) {
  const double a = std::abs(delta);
  if (a <= tol) return FlagState::Ok;
  if (a <= 2.0 * tol) return FlagState::Review;
  return FlagState::OutsideTarget;
}
}  // namespace

FlagState loudnessFlag(const AnalysisResult& r, const TargetProfile& p) {
  return classifyDelta(r.integratedLufs - p.loudnessTargetLufs, 1.0);  // ±1 LU ok
}

FlagState dynamicRangeFlag(const AnalysisResult& r, const TargetProfile& p) {
  const double tol = p.drTolerance.plusMinus > 0 ? p.drTolerance.plusMinus : 2.0;
  return classifyDelta(r.drValue - p.drTolerance.target, tol);
}

FlagState monoFlag(const AnalysisResult& r, const TargetProfile& p) {
  if (!r.phaseCorrelation.has_value()) return FlagState::Ok;  // mono track
  const double corr = *r.phaseCorrelation;
  if (corr >= p.monoSafetyThreshold) return FlagState::Ok;
  if (corr >= p.monoSafetyThreshold - 0.2) return FlagState::Review;
  return FlagState::OutsideTarget;
}

FlagState widthFlag(const AnalysisResult& r, const TargetProfile& p) {
  if (!r.stereoWidth.has_value()) return FlagState::Ok;
  const double tol = p.widthTolerance.plusMinus > 0 ? p.widthTolerance.plusMinus : 0.25;
  return classifyDelta(*r.stereoWidth - p.widthTolerance.target, tol);
}

const char* flagLabel(FlagState s) {
  switch (s) {
    case FlagState::Ok: return "OK";
    case FlagState::Review: return "Review";
    case FlagState::OutsideTarget: return "Outside Target";
  }
  return "?";
}

}  // namespace djcore
