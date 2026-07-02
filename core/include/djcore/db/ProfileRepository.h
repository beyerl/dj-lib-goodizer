#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "djcore/model/TargetProfile.h"

namespace djcore {

class Database;

// CRUD for named target profiles, plus seeding of the shipped defaults.
class ProfileRepository {
 public:
  explicit ProfileRepository(Database& db) : db_(db) {}

  std::int64_t insert(const TargetProfile& profile);
  std::vector<TargetProfile> all();
  std::optional<TargetProfile> getByName(const std::string& name);
  void seedDefaultsIfEmpty();

 private:
  Database& db_;
};

}  // namespace djcore
