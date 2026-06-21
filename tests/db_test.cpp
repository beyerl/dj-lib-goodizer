#include <gtest/gtest.h>

#include "djcore/db/AnalysisResultRepository.h"
#include "djcore/db/Database.h"
#include "djcore/db/ProfileRepository.h"
#include "djcore/db/Schema.h"
#include "djcore/db/TrackRepository.h"

using namespace djcore;

namespace {
Track makeTrack(const std::string& path) {
  Track t;
  t.sourcePath = path;
  t.format.container = "wav";
  t.format.codec = "pcm_s16le";
  t.format.sampleRate = 44100;
  t.format.bitDepth = 16;
  t.format.channels = 2;
  t.format.durationMs = 180000;
  t.importedAtUnix = 1700000000;
  return t;
}
}  // namespace

TEST(Db, MigrateIsIdempotent) {
  Database db(":memory:");
  migrate(db);
  migrate(db);  // second call must be a no-op, not an error
  Statement s = db.prepare("PRAGMA user_version;");
  s.step();
  EXPECT_EQ(s.columnInt64(0), kSchemaVersion);
}

TEST(Db, TrackInsertGetCount) {
  Database db(":memory:");
  migrate(db);
  TrackRepository repo(db);

  const std::int64_t id = repo.insert(makeTrack("/music/a.wav"));
  EXPECT_GT(id, 0);
  EXPECT_EQ(repo.count(), 1);

  auto got = repo.getById(id);
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(got->sourcePath, "/music/a.wav");
  EXPECT_EQ(got->format.sampleRate, 44100);
  EXPECT_EQ(got->format.channels, 2);

  repo.insert(makeTrack("/music/b.wav"));
  EXPECT_EQ(repo.count(), 2);
  EXPECT_EQ(repo.all().size(), 2u);
}

TEST(Db, AnalysisResultUpsertPreservesOptionalsAndReplaces) {
  Database db(":memory:");
  migrate(db);
  TrackRepository tracks(db);
  AnalysisResultRepository results(db);

  const std::int64_t id = tracks.insert(makeTrack("/music/c.wav"));

  AnalysisResult r;
  r.leadingSilenceMs = 250;
  r.integratedLufs = -14.0;
  r.crestFactor = 1.414;
  r.phaseCorrelation = 0.8;       // set
  r.stereoWidth = std::nullopt;   // unset -> stored NULL
  r.analyzerVersion = 1;
  results.upsert(id, r);

  auto got = results.getByTrack(id);
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(got->leadingSilenceMs, 250);
  EXPECT_NEAR(got->integratedLufs, -14.0, 1e-9);
  ASSERT_TRUE(got->phaseCorrelation.has_value());
  EXPECT_NEAR(*got->phaseCorrelation, 0.8, 1e-9);
  EXPECT_FALSE(got->stereoWidth.has_value());

  // Re-analysis replaces the row (UNIQUE track_id), not appends.
  r.integratedLufs = -10.0;
  results.upsert(id, r);
  got = results.getByTrack(id);
  ASSERT_TRUE(got.has_value());
  EXPECT_NEAR(got->integratedLufs, -10.0, 1e-9);
}

TEST(Db, ProfileSeedAndRoundTrip) {
  Database db(":memory:");
  migrate(db);
  ProfileRepository repo(db);

  repo.seedDefaultsIfEmpty();
  repo.seedDefaultsIfEmpty();  // must not duplicate

  const auto profiles = repo.all();
  EXPECT_GE(profiles.size(), 3u);

  auto club = repo.getByName("Club / USB");
  ASSERT_TRUE(club.has_value());
  EXPECT_EQ(club->sampleRate, 44100);
  EXPECT_EQ(club->bitDepth, 16);
  EXPECT_NEAR(club->loudnessTargetLufs, -14.0, 1e-9);
  EXPECT_NEAR(club->drTolerance.target, 8.0, 1e-9);
}
