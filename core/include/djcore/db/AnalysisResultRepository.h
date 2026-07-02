#pragma once

#include <cstdint>
#include <optional>

#include "djcore/model/AnalysisResult.h"

namespace djcore {

class Database;

// Per-track analysis metrics. upsert() is idempotent (one row per track).
class AnalysisResultRepository {
 public:
  explicit AnalysisResultRepository(Database& db) : db_(db) {}

  void upsert(std::int64_t trackId, const AnalysisResult& result);
  std::optional<AnalysisResult> getByTrack(std::int64_t trackId);

 private:
  Database& db_;
};

}  // namespace djcore
