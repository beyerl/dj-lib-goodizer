#include "djcore/db/TrackRepository.h"

#include "djcore/db/Database.h"

namespace djcore {
namespace {

Track readTrack(Statement& s) {
  Track t;
  t.id = s.columnInt64(0);
  t.sourcePath = s.columnText(1);
  t.outputPath = s.columnText(2);
  t.format.container = s.columnText(3);
  t.format.codec = s.columnText(4);
  t.format.sampleRate = static_cast<int>(s.columnInt64(5));
  t.format.bitDepth = static_cast<int>(s.columnInt64(6));
  t.format.channels = static_cast<int>(s.columnInt64(7));
  t.format.durationMs = s.columnInt64(8);
  t.format.sourceIsLossy = s.columnInt64(9) != 0;
  t.importedAtUnix = s.columnInt64(10);
  return t;
}

constexpr const char* kCols =
    "id, source_path, output_path, container, codec, sample_rate, bit_depth, "
    "channels, duration_ms, source_lossy, imported_at";

}  // namespace

std::int64_t TrackRepository::insert(const Track& t) {
  Statement s = db_.prepare(
      "INSERT INTO track (source_path, output_path, container, codec, "
      "sample_rate, bit_depth, channels, duration_ms, source_lossy, imported_at) "
      "VALUES (?,?,?,?,?,?,?,?,?,?);");
  s.bind(1, t.sourcePath);
  if (t.outputPath.empty()) s.bindNull(2); else s.bind(2, t.outputPath);
  s.bind(3, t.format.container);
  s.bind(4, t.format.codec);
  s.bind(5, static_cast<std::int64_t>(t.format.sampleRate));
  s.bind(6, static_cast<std::int64_t>(t.format.bitDepth));
  s.bind(7, static_cast<std::int64_t>(t.format.channels));
  s.bind(8, t.format.durationMs);
  s.bind(9, static_cast<std::int64_t>(t.format.sourceIsLossy ? 1 : 0));
  s.bind(10, t.importedAtUnix);
  s.step();
  return db_.lastInsertRowId();
}

std::optional<Track> TrackRepository::getById(std::int64_t id) {
  Statement s = db_.prepare(std::string("SELECT ") + kCols +
                            " FROM track WHERE id=?;");
  s.bind(1, id);
  if (!s.step()) return std::nullopt;
  return readTrack(s);
}

std::vector<Track> TrackRepository::all() {
  Statement s = db_.prepare(std::string("SELECT ") + kCols +
                            " FROM track ORDER BY id;");
  std::vector<Track> out;
  while (s.step()) out.push_back(readTrack(s));
  return out;
}

std::int64_t TrackRepository::count() {
  Statement s = db_.prepare("SELECT COUNT(*) FROM track;");
  s.step();
  return s.columnInt64(0);
}

void TrackRepository::setOutputPath(std::int64_t id, const std::string& outputPath) {
  Statement s = db_.prepare("UPDATE track SET output_path=? WHERE id=?;");
  s.bind(1, outputPath);
  s.bind(2, id);
  s.step();
}

}  // namespace djcore
