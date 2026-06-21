#pragma once

#include <cstdint>
#include <optional>

#include "djcore/model/AnalysisResult.h"

namespace djcore {

class Database;

// Persistence for per-track analysis metrics. One result per track (latest);
// re-analysis replaces the prior row.
class AnalysisResultRepository {
 public:
  explicit AnalysisResultRepository(Database& db) : db_(db) {}

  void upsert(std::int64_t trackId, const AnalysisResult& result);
  std::optional<AnalysisResult> getByTrack(std::int64_t trackId);

 private:
  Database& db_;
};

}  // namespace djcore
