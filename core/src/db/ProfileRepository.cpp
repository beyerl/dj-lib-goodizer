#include "djcore/db/ProfileRepository.h"

#include "djcore/db/Database.h"
#include "djcore/profile/DefaultProfiles.h"

namespace djcore {
namespace {

const char* kSelectCols =
    "id, name, container, sample_rate, bit_depth, silence_threshold_db, "
    "silence_min_ms, silence_lead_in_ms, loudness_target, dr_target, dr_plusminus, "
    "width_target, width_plusminus, mono_threshold";

TargetProfile readRow(Statement& st) {
  TargetProfile p;
  p.id = st.columnInt64(0);
  p.name = st.columnText(1);
  p.container = st.columnText(2);
  p.sampleRate = static_cast<int>(st.columnInt64(3));
  p.bitDepth = static_cast<int>(st.columnInt64(4));
  p.silence.thresholdDb = st.columnDouble(5);
  p.silence.minDurationMs = static_cast<int>(st.columnInt64(6));
  p.silence.leadInMs = static_cast<int>(st.columnInt64(7));
  p.loudnessTargetLufs = st.columnDouble(8);
  p.drTolerance.target = st.columnDouble(9);
  p.drTolerance.plusMinus = st.columnDouble(10);
  p.widthTolerance.target = st.columnDouble(11);
  p.widthTolerance.plusMinus = st.columnDouble(12);
  p.monoSafetyThreshold = st.columnDouble(13);
  return p;
}

}  // namespace

std::int64_t ProfileRepository::insert(const TargetProfile& p) {
  Statement st = db_.prepare(
      "INSERT INTO target_profile (name, container, sample_rate, bit_depth, "
      "silence_threshold_db, silence_min_ms, silence_lead_in_ms, loudness_target, "
      "dr_target, dr_plusminus, width_target, width_plusminus, mono_threshold) "
      "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?);");
  st.bind(1, p.name);
  st.bind(2, p.container);
  st.bind(3, static_cast<std::int64_t>(p.sampleRate));
  st.bind(4, static_cast<std::int64_t>(p.bitDepth));
  st.bind(5, p.silence.thresholdDb);
  st.bind(6, static_cast<std::int64_t>(p.silence.minDurationMs));
  st.bind(7, static_cast<std::int64_t>(p.silence.leadInMs));
  st.bind(8, p.loudnessTargetLufs);
  st.bind(9, p.drTolerance.target);
  st.bind(10, p.drTolerance.plusMinus);
  st.bind(11, p.widthTolerance.target);
  st.bind(12, p.widthTolerance.plusMinus);
  st.bind(13, p.monoSafetyThreshold);
  st.step();
  return db_.lastInsertRowId();
}

std::vector<TargetProfile> ProfileRepository::all() {
  Statement st = db_.prepare(std::string("SELECT ") + kSelectCols +
                             " FROM target_profile ORDER BY id;");
  std::vector<TargetProfile> out;
  while (st.step()) out.push_back(readRow(st));
  return out;
}

std::optional<TargetProfile> ProfileRepository::getByName(const std::string& name) {
  Statement st = db_.prepare(std::string("SELECT ") + kSelectCols +
                             " FROM target_profile WHERE name=?;");
  st.bind(1, name);
  if (st.step()) return readRow(st);
  return std::nullopt;
}

void ProfileRepository::seedDefaultsIfEmpty() {
  Statement st = db_.prepare("SELECT COUNT(*) FROM target_profile;");
  st.step();
  if (st.columnInt64(0) > 0) return;
  for (const auto& p : defaultProfiles()) insert(p);
}

}  // namespace djcore
