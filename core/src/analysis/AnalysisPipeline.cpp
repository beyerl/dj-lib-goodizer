#include "djcore/analysis/AnalysisPipeline.h"

#include <algorithm>
#include <ctime>
#include <vector>

#include "djcore/analysis/analyzers/LoudnessDynamicsAnalyzer.h"
#include "djcore/analysis/analyzers/PhaseMonoAnalyzer.h"
#include "djcore/analysis/analyzers/SilenceDetector.h"
#include "djcore/analysis/analyzers/StereoWidthAnalyzer.h"
#include "djcore/audio/AudioDecoder.h"

namespace djcore {

AnalysisPipeline::AnalysisPipeline(const AnalysisConfig& config) {
  analyzers_.push_back(std::make_unique<SilenceDetector>(config));
  analyzers_.push_back(std::make_unique<LoudnessDynamicsAnalyzer>());
  analyzers_.push_back(std::make_unique<PhaseMonoAnalyzer>());
  analyzers_.push_back(std::make_unique<StereoWidthAnalyzer>());
}

void AnalysisPipeline::addAnalyzer(std::unique_ptr<IAnalyzer> analyzer) {
  analyzers_.push_back(std::move(analyzer));
}

AnalysisResult AnalysisPipeline::analyze(AudioDecoder& decoder) {
  const FormatInfo fmt = decoder.format();
  PcmBuffer buffer = decoder.readAll();

  for (auto& a : analyzers_) a->begin(fmt);

  const int ch = buffer.channels();
  const std::size_t total = buffer.frames();
  constexpr std::size_t kBlock = 8192;
  std::vector<const float*> planes(ch > 0 ? static_cast<std::size_t>(ch) : 0);

  for (std::size_t off = 0; off < total; off += kBlock) {
    const std::size_t n = std::min(kBlock, total - off);
    for (int c = 0; c < ch; ++c) planes[c] = buffer.channel(c) + off;
    PcmBlock blk;
    blk.planes = planes.data();
    blk.channels = ch;
    blk.sampleRate = buffer.sampleRate();
    blk.frames = n;
    for (auto& a : analyzers_) a->processBlock(blk);
  }

  AnalysisResult result;
  for (auto& a : analyzers_) a->finalize(result);
  result.analyzerVersion = kAnalyzerVersion;
  result.analyzedAtUnix = static_cast<std::int64_t>(std::time(nullptr));
  return result;
}

AnalysisResult AnalysisPipeline::analyzeFile(const std::string& path) {
  auto decoder = openDecoder(path);
  return analyze(*decoder);
}

}  // namespace djcore
