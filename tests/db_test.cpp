#include <ctime>
#include <filesystem>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include "djcore/db/AuditLogRepository.h"
#include "djcore/db/Database.h"
#include "djcore/db/Schema.h"
#include "djcore/db/TrackRepository.h"

namespace fs = std::filesystem;
using namespace djcore;

TEST(Db, MigrateIsIdempotentAndSetsVersion) {
  Database db(":memory:");
  migrate(db);
  migrate(db);  // second run must be a no-op, not an error
  EXPECT_EQ(db.userVersion(), kSchemaVersion);
}

TEST(Db, TrackInsertGetCountUpdate) {
  Database db(":memory:");
  migrate(db);
  TrackRepository repo(db);

  Track t;
  t.sourcePath = "/music/a.wav";
  t.format.container = "wav";
  t.format.codec = "pcm_s16le";
  t.format.sampleRate = 44100;
  t.format.bitDepth = 16;
  t.format.channels = 2;
  t.format.durationMs = 1000;

  const std::int64_t id = repo.insert(t);
  EXPECT_GT(id, 0);
  EXPECT_EQ(repo.count(), 1);

  auto got = repo.getById(id);
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(got->sourcePath, "/music/a.wav");
  EXPECT_EQ(got->format.sampleRate, 44100);
  EXPECT_EQ(got->format.channels, 2);
  EXPECT_TRUE(got->outputPath.empty());

  repo.setOutputPath(id, "/out/a.wav");
  EXPECT_EQ(repo.getById(id)->outputPath, "/out/a.wav");
}

TEST(Db, AuditInsertAndQuery) {
  Database db(":memory:");
  migrate(db);
  TrackRepository tr(db);
  AuditLogRepository ar(db);

  Track t;
  t.sourcePath = "/music/a.wav";
  const std::int64_t id = tr.insert(t);

  AuditLogEntry e;
  e.trackId = id;
  e.operation = OperationType::ResampleTranscode;
  e.afterJson = "{\"sampleRate\":44100}";
  e.timestampUnix = 123;
  ar.insert(e);

  EXPECT_EQ(ar.count(), 1);
  ASSERT_EQ(ar.forTrack(id).size(), 1u);
  EXPECT_EQ(ar.forTrack(id)[0].afterJson, "{\"sampleRate\":44100}");
}

// The historical "database is locked" bug: two connections writing the same
// file DB concurrently. WAL + busy_timeout must let them both succeed.
TEST(Db, ConcurrentWritersDoNotLock) {
  const auto dbPath =
      (fs::temp_directory_path() /
       ("djtest_conc_" + std::to_string(static_cast<long long>(::time(nullptr))) +
        ".db"))
          .string();
  {
    Database init(dbPath);
    migrate(init);
  }

  auto writer = [&](int base) {
    Database db(dbPath);
    TrackRepository repo(db);
    for (int i = 0; i < 50; ++i) {
      Track t;
      t.sourcePath = "/x/" + std::to_string(base + i) + ".wav";
      repo.insert(t);
    }
  };

  std::thread t1(writer, 0);
  std::thread t2(writer, 1000);
  t1.join();
  t2.join();

  {
    Database db(dbPath);
    TrackRepository repo(db);
    EXPECT_EQ(repo.count(), 100);
  }

  std::error_code ec;
  fs::remove(dbPath, ec);
  fs::remove(dbPath + "-wal", ec);
  fs::remove(dbPath + "-shm", ec);
}
