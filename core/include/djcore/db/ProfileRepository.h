#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "djcore/model/TargetProfile.h"

namespace djcore {

class Database;

// Persistence for named standardization profiles (NFR-EXT-2). Nested structs
// (silence policy, tolerance bands) are stored as small JSON blobs.
class ProfileRepository {
 public:
  explicit ProfileRepository(Database& db) : db_(db) {}

  std::int64_t insert(const TargetProfile& profile);
  std::vector<TargetProfile> all();
  std::optional<TargetProfile> getByName(const std::string& name);

  // Inserts the shipped default profiles if the table is empty (FR-FMT-3).
  void seedDefaultsIfEmpty();

 private:
  Database& db_;
};

}  // namespace djcore
