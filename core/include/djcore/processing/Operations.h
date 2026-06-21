#pragma once

#include <cstddef>

#include "djcore/audio/PcmBuffer.h"
#include "djcore/model/Enums.h"
#include "djcore/processing/IOperation.h"

namespace djcore {

// Gain-only loudness adjustment (FR-DR-5). Scales every sample by a constant
// linear factor, so peak and RMS scale together — crest factor and dynamic
// range are unchanged (AC 3.3.3). Categorized Standardizing.
class GainOperation : public IOperation {
 public:
  explicit GainOperation(double gainLinear) : gain_(gainLinear) {}

  const char* name() const override { return "gain-normalize"; }
  OperationType type() const override { return OperationType::GainNormalize; }
  OperationCategory category() const override { return OperationCategory::Standardizing; }
  void apply(PcmBuffer& buffer) override;

  double gainLinear() const { return gain_; }

 private:
  double gain_;
};

// Returns a copy of `in` with `startFrames` removed from the front and
// `endFrames` from the back (FR-SIL-4). Cuts are snapped toward the nearest
// zero crossing within a short window to avoid clicks at the new boundary.
PcmBuffer trimSilence(const PcmBuffer& in, std::size_t startFrames,
                      std::size_t endFrames);

}  // namespace djcore
