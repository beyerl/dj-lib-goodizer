#pragma once

#include "djcore/audio/PcmBuffer.h"
#include "djcore/model/Enums.h"

namespace djcore {

// A single in-place PCM transform. Standardizing ops (resample, trim, gain) are
// applied by default; PerceptualAltering ops (compression, width/balance) are
// opt-in only.
class IOperation {
 public:
  virtual ~IOperation() = default;
  virtual const char* name() const = 0;
  virtual OperationType type() const = 0;
  virtual OperationCategory category() const = 0;
  virtual void apply(PcmBuffer& buffer) = 0;
};

}  // namespace djcore
