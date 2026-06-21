#include "djcore/db/ProfileRepository.h"

#include <string>

#include "djcore/db/Database.h"
#include "djcore/profile/DefaultProfiles.h"

namespace djcore {
namespace {

// Minimal JSON helpers for the fixed-shape nested structs (avoids a JSON
// dependency). Values are simple numbers keyed by name.
std::string toJson(const SilencePolicy& s) {
  return "{\"thresholdDb\":" + std::to_string(s.thresholdDb) +
         ",\"minDurationMs\":" + std::to_string(s.minDurationMs) +
         ",\"leadInMs\":" + std::to_string(s.leadInMs) + "}";
}
std::string toJson(const ToleranceBand& b) {
  return "{\"target\":" + std::to_string(b.target) +
         ",\"plusMinus\":" + std::to_string(b.plusMinus) + "}";
}

// Reads the number following "key": in a flat JSON object. Returns fallback if
// the key is absent.
double jsonNumber(const std::string& json, const std::string& key, double fallback) {
  const std::string needle = "\"" + key + "\":";
  const auto pos = json.find(needle);
  if (pos == std::string::npos) return fallback;
  try {
    return std::stod(json.substr(pos + needle.size()));
  } catch (...) {
    return fallback;
  }
}

SilencePolicy silenceFromJson(const std::string& j) {
  SilencePolicy s;
  s.thresholdDb = jsonNumber(j, "thresholdDb", s.thresholdDb);
  s.minDurationMs = static_cast<int>(jsonNumber(j, "minDurationMs", s.minDurationMs));
  s.leadInMs = static_cast<int>(jsonNumber(j, "leadInMs", s.leadInMs));
  return s;
}
ToleranceBand bandFromJson(const std::string& j) {
  ToleranceBand b;
  b.target = jsonNumber(j, "target", b.target);
  b.plusMinus = jsonNumber(j, "plusMinus", b.plusMinus);
  return b;
}

}  // namespace

std::int64_t ProfileRepository::insert(const TargetProfile& p) {
  Statement s = db_.prepare(
      "INSERT INTO target_profile (name, container, sample_rate, bit_depth, "
      "silence_json, loudness_target, dr_tolerance_json, mono_threshold, "
      "width_tolerance_json) VALUES (?,?,?,?,?,?,?,?,?);");
  s.bind(1, p.name);
  s.bind(2, p.container);
  s.bind(3, static_cast<std::int64_t>(p.sampleRate));
  s.bind(4, static_cast<std::int64_t>(p.bitDepth));
  s.bind(5, toJson(p.silence));
  s.bind(6, p.loudnessTargetLufs);
  s.bind(7, toJson(p.drTolerance));
  s.bind(8, p.monoSafetyThreshold);
  s.bind(9, toJson(p.widthTolerance));
  s.step();
  return db_.lastInsertRowId();
}

namespace {
TargetProfile readProfile(Statement& s) {
  TargetProfile p;
  p.id = s.columnInt64(0);
  p.name = s.columnText(1);
  p.container = s.columnText(2);
  p.sampleRate = static_cast<int>(s.columnInt64(3));
  p.bitDepth = static_cast<int>(s.columnInt64(4));
  p.silence = silenceFromJson(s.columnText(5));
  p.loudnessTargetLufs = s.columnDouble(6);
  p.drTolerance = bandFromJson(s.columnText(7));
  p.monoSafetyThreshold = s.columnDouble(8);
  p.widthTolerance = bandFromJson(s.columnText(9));
  return p;
}
constexpr const char* kCols =
    "id, name, container, sample_rate, bit_depth, silence_json, "
    "loudness_target, dr_tolerance_json, mono_threshold, width_tolerance_json";
}  // namespace

std::vector<TargetProfile> ProfileRepository::all() {
  Statement s = db_.prepare(std::string("SELECT ") + kCols +
                            " FROM target_profile ORDER BY id;");
  std::vector<TargetProfile> out;
  while (s.step()) out.push_back(readProfile(s));
  return out;
}

std::optional<TargetProfile> ProfileRepository::getByName(const std::string& name) {
  Statement s = db_.prepare(std::string("SELECT ") + kCols +
                            " FROM target_profile WHERE name=?;");
  s.bind(1, name);
  if (!s.step()) return std::nullopt;
  return readProfile(s);
}

void ProfileRepository::seedDefaultsIfEmpty() {
  Statement count = db_.prepare("SELECT COUNT(*) FROM target_profile;");
  count.step();
  if (count.columnInt64(0) > 0) return;
  for (const auto& p : defaultProfiles()) {
    insert(p);
  }
}

}  // namespace djcore
