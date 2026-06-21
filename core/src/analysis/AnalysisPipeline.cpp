#include "djcore/analysis/AnalysisPipeline.h"

#include <vector>

#include "djcore/Version.h"
#include "djcore/analysis/analyzers/LoudnessDynamicsAnalyzer.h"
#include "djcore/analysis/analyzers/PhaseMonoAnalyzer.h"
#include "djcore/analysis/analyzers/SilenceDetector.h"
#include "djcore/analysis/analyzers/StereoWidthAnalyzer.h"
#include "djcore/audio/PcmBuffer.h"

namespace djcore {

namespace {
constexpr std::size_t kBlockFrames = 8192;
}

AnalysisPipeline::AnalysisPipeline(const AnalysisConfig& cfg) {
  analyzers_.push_back(std::make_unique<SilenceDetector>(cfg));
  analyzers_.push_back(std::make_unique<LoudnessDynamicsAnalyzer>());
  analyzers_.push_back(std::make_unique<PhaseMonoAnalyzer>());
  analyzers_.push_back(std::make_unique<StereoWidthAnalyzer>());
}

void AnalysisPipeline::addAnalyzer(std::unique_ptr<IAnalyzer> analyzer) {
  analyzers_.push_back(std::move(analyzer));
}

AnalysisResult AnalysisPipeline::analyze(AudioDecoder& decoder) {
  const FormatInfo& format = decoder.format();
  for (auto& a : analyzers_) {
    a->begin(format);
  }

  PcmBuffer block;
  std::vector<const float*> planes;
  while (decoder.readBlock(block, kBlockFrames) > 0) {
    planes.resize(static_cast<std::size_t>(block.channels()));
    for (int ch = 0; ch < block.channels(); ++ch) {
      planes[static_cast<std::size_t>(ch)] = block.channel(ch);
    }
    PcmBlock pb;
    pb.planes = planes.data();
    pb.channels = block.channels();
    pb.sampleRate = block.sampleRate();
    pb.frames = block.frames();
    for (auto& a : analyzers_) {
      a->processBlock(pb);
    }
  }

  AnalysisResult result;
  result.analyzerVersion = kAnalyzerVersion;
  for (auto& a : analyzers_) {
    a->finalize(result);
  }
  return result;
}

AnalysisResult AnalysisPipeline::analyzeFile(const std::string& path) {
  auto decoder = openDecoder(path);
  return analyze(*decoder);
}

}  // namespace djcore
