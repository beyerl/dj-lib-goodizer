#pragma once

#include "djcore/audio/PcmBuffer.h"
#include "djcore/model/Enums.h"

namespace djcore {

// A single processing operation in the per-track operation chain (plan §3).
// The category() is enforced when assembling chains: PerceptualAltering ops are
// only included when the user has explicitly opted in for the job.
class IOperation {
 public:
  virtual ~IOperation() = default;

  virtual const char* name() const = 0;
  virtual OperationType type() const = 0;
  virtual OperationCategory category() const = 0;

  // Transform the buffer in place. (Concrete implementations land in M2/M6;
  // the skeleton only defines the contract.)
  virtual void apply(PcmBuffer& buffer) = 0;
};

}  // namespace djcore
