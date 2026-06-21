#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "djcore/model/Track.h"

namespace djcore {

class Database;

// Persistence for imported tracks (spec Table 11).
class TrackRepository {
 public:
  explicit TrackRepository(Database& db) : db_(db) {}

  std::int64_t insert(const Track& track);
  std::optional<Track> getById(std::int64_t id);
  std::vector<Track> all();
  std::int64_t count();
  void setOutputPath(std::int64_t id, const std::string& outputPath);

 private:
  Database& db_;
};

}  // namespace djcore
