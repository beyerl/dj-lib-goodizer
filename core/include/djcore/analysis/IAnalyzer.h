#pragma once

#include "djcore/audio/FormatInfo.h"
#include "djcore/audio/PcmBuffer.h"
#include "djcore/model/AnalysisResult.h"

namespace djcore {

// A stateless-config, streaming analysis module (plan §2). The analysis
// pipeline decodes each file once and fans every PCM block to all registered
// analyzers, so no file is decoded more than once.
//
//   begin(format)         -> reset accumulators for a new track
//   processBlock(block)   -> called repeatedly with consecutive PCM blocks
//   finalize(result)      -> write this module's metrics into the shared result
class IAnalyzer {
 public:
  virtual ~IAnalyzer() = default;

  virtual const char* name() const = 0;
  virtual void begin(const FormatInfo& format) = 0;
  virtual void processBlock(const PcmBlock& block) = 0;
  virtual void finalize(AnalysisResult& result) = 0;
};

}  // namespace djcore
