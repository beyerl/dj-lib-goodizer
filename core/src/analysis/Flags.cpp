#include "djcore/analysis/Flags.h"

#include <cmath>

namespace djcore {
namespace {

// Ok within ±plusMinus, Review within ±2*plusMinus, else OutsideTarget.
FlagState bandFlag(double value, double target, double plusMinus) {
  if (plusMinus <= 0.0) return FlagState::Ok;  // no target configured
  const double d = std::fabs(value - target);
  if (d <= plusMinus) return FlagState::Ok;
  if (d <= 2.0 * plusMinus) return FlagState::Review;
  return FlagState::OutsideTarget;
}

}  // namespace

FlagState loudnessFlag(const AnalysisResult& r, const TargetProfile& p) {
  if (p.loudnessTargetLufs == 0.0) return FlagState::Ok;
  return bandFlag(r.integratedLufs, p.loudnessTargetLufs, 1.0);
}

FlagState dynamicRangeFlag(const AnalysisResult& r, const TargetProfile& p) {
  return bandFlag(r.drValue, p.drTolerance.target, p.drTolerance.plusMinus);
}

FlagState monoFlag(const AnalysisResult& r, const TargetProfile& p) {
  if (!r.phaseCorrelation) return FlagState::Ok;  // mono track
  const double rho = *r.phaseCorrelation;
  if (rho >= p.monoSafetyThreshold) return FlagState::Ok;
  if (rho >= p.monoSafetyThreshold - 0.2) return FlagState::Review;
  return FlagState::OutsideTarget;
}

FlagState widthFlag(const AnalysisResult& r, const TargetProfile& p) {
  if (!r.stereoWidth) return FlagState::Ok;  // mono track
  return bandFlag(*r.stereoWidth, p.widthTolerance.target, p.widthTolerance.plusMinus);
}

const char* flagLabel(FlagState state) {
  switch (state) {
    case FlagState::Ok:
      return "OK";
    case FlagState::Review:
      return "Review";
    case FlagState::OutsideTarget:
      return "Outside Target";
  }
  return "?";
}

}  // namespace djcore
