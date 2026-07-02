#pragma once

#include "djcore/audio/FormatInfo.h"
#include "djcore/audio/PcmBuffer.h"
#include "djcore/model/AnalysisResult.h"

namespace djcore {

// A streaming, stateless-across-files analyzer. The pipeline decodes each file
// once and fans consecutive, non-overlapping PcmBlocks to every analyzer:
//   begin(format) -> processBlock(...) x N -> finalize(result)
// Analyzers must carry any windowing/gating state across processBlock calls and
// must not assume a fixed block size. They write disjoint fields of the shared
// AnalysisResult, so registration order does not matter.
class IAnalyzer {
 public:
  virtual ~IAnalyzer() = default;
  virtual const char* name() const = 0;
  virtual void begin(const FormatInfo& format) = 0;
  virtual void processBlock(const PcmBlock& block) = 0;
  virtual void finalize(AnalysisResult& result) = 0;
};

}  // namespace djcore
