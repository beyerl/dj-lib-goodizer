#pragma once

#include <cstddef>

#include "djcore/audio/PcmBuffer.h"
#include "djcore/processing/IOperation.h"

namespace djcore {

// Constant linear gain applied to every sample. Scales peak and RMS equally, so
// crest factor / dynamic range are unchanged (Standardizing, not perceptual).
class GainOperation : public IOperation {
 public:
  explicit GainOperation(double gainLinear) : gain_(gainLinear) {}
  const char* name() const override { return "gain"; }
  OperationType type() const override { return OperationType::GainNormalize; }
  OperationCategory category() const override {
    return OperationCategory::Standardizing;
  }
  void apply(PcmBuffer& buffer) override;

 private:
  double gain_;
};

// Trims `startFrames` from the front and `endFrames` from the back, snapping
// each cut to the nearest zero crossing within ~1 ms to avoid clicks.
PcmBuffer trimSilence(const PcmBuffer& in, std::size_t startFrames,
                      std::size_t endFrames);

}  // namespace djcore
