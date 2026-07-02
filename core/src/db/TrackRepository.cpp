#include "djcore/db/TrackRepository.h"

#include "djcore/db/Database.h"

namespace djcore {
namespace {

Track readRow(Statement& st) {
  Track t;
  t.id = st.columnInt64(0);
  t.sourcePath = st.columnText(1);
  t.outputPath = st.columnIsNull(2) ? std::string{} : st.columnText(2);
  t.format.container = st.columnText(3);
  t.format.codec = st.columnText(4);
  t.format.sampleRate = static_cast<int>(st.columnInt64(5));
  t.format.bitDepth = static_cast<int>(st.columnInt64(6));
  t.format.channels = static_cast<int>(st.columnInt64(7));
  t.format.durationMs = st.columnInt64(8);
  t.format.sourceIsLossy = st.columnInt64(9) != 0;
  t.importedAtUnix = st.columnInt64(10);
  return t;
}

const char* kSelectCols =
    "id, source_path, output_path, container, codec, sample_rate, bit_depth, "
    "channels, duration_ms, source_lossy, imported_at";

}  // namespace

std::int64_t TrackRepository::insert(const Track& track) {
  Statement st = db_.prepare(
      "INSERT INTO track (source_path, output_path, container, codec, "
      "sample_rate, bit_depth, channels, duration_ms, source_lossy, imported_at) "
      "VALUES (?,?,?,?,?,?,?,?,?,?);");
  st.bind(1, track.sourcePath);
  if (track.outputPath.empty())
    st.bindNull(2);
  else
    st.bind(2, track.outputPath);
  st.bind(3, track.format.container);
  st.bind(4, track.format.codec);
  st.bind(5, static_cast<std::int64_t>(track.format.sampleRate));
  st.bind(6, static_cast<std::int64_t>(track.format.bitDepth));
  st.bind(7, static_cast<std::int64_t>(track.format.channels));
  st.bind(8, track.format.durationMs);
  st.bind(9, static_cast<std::int64_t>(track.format.sourceIsLossy ? 1 : 0));
  st.bind(10, track.importedAtUnix);
  st.step();
  return db_.lastInsertRowId();
}

std::optional<Track> TrackRepository::getById(std::int64_t id) {
  Statement st = db_.prepare(std::string("SELECT ") + kSelectCols +
                             " FROM track WHERE id=?;");
  st.bind(1, id);
  if (st.step()) return readRow(st);
  return std::nullopt;
}

std::vector<Track> TrackRepository::all() {
  Statement st =
      db_.prepare(std::string("SELECT ") + kSelectCols + " FROM track ORDER BY id;");
  std::vector<Track> out;
  while (st.step()) out.push_back(readRow(st));
  return out;
}

std::int64_t TrackRepository::count() {
  Statement st = db_.prepare("SELECT COUNT(*) FROM track;");
  st.step();
  return st.columnInt64(0);
}

void TrackRepository::setOutputPath(std::int64_t id, const std::string& outputPath) {
  Statement st = db_.prepare("UPDATE track SET output_path=? WHERE id=?;");
  st.bind(1, outputPath);
  st.bind(2, id);
  st.step();
}

}  // namespace djcore
