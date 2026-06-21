// Exercises the real background import lifecycle (ImportSession + a worker
// thread) end-to-end. This is the regression guard for the "import progress bar
// freezes" bug: a GUI-thread deadlock would manifest here as the event loop
// never returning, which the watchdog turns into a hard test failure rather
// than a hang. Runs on the desktop CI runners (Linux + Windows) where Qt is
// available.
#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QString>
#include <QTimer>
#include <cmath>
#include <filesystem>
#include <string>

#include "ImportSession.h"
#include "djcore/audio/AudioEncoder.h"
#include "djcore/audio/PcmBuffer.h"
#include "djcore/db/Database.h"
#include "djcore/db/Schema.h"
#include "djcore/db/TrackRepository.h"

namespace fs = std::filesystem;
using namespace djcore;

namespace {

constexpr double kPi = 3.14159265358979323846;

PcmBuffer makeSine(int sampleRate, std::size_t frames) {
  PcmBuffer buf(2, sampleRate, frames);
  for (std::size_t i = 0; i < frames; ++i) {
    const double t = static_cast<double>(i) / sampleRate;
    const float v = static_cast<float>(0.4 * std::sin(2.0 * kPi * 440.0 * t));
    buf.channel(0)[i] = v;
    buf.channel(1)[i] = v;
  }
  return buf;
}

}  // namespace

TEST(ImportSession, ThreadedImportCompletesWithoutDeadlock) {
  const fs::path dir = fs::temp_directory_path() / "djimport_session_test";
  fs::remove_all(dir);
  fs::create_directories(dir);

  constexpr int kFiles = 3;
  for (int i = 0; i < kFiles; ++i) {
    const PcmBuffer pcm = makeSine(44100, 8192);
    encodeToFile((dir / ("track" + std::to_string(i) + ".wav")).string(), pcm,
                 EncodeSpec{"wav", 44100, 16, /*isFloat=*/false});
  }

  const std::string dbPath = (dir / "library.db").string();
  {
    Database db(dbPath);  // create + migrate schema, then release the connection
    migrate(db);
  }

  static char arg0[] = "djapp_tests";
  static char* argv[] = {arg0, nullptr};
  int argc = 1;
  QCoreApplication app(argc, argv);

  djapp::ImportSession session;
  int gotImported = -1, gotFailed = -1, progressCount = 0;
  bool finished = false, timedOut = false;

  QObject::connect(&session, &djapp::ImportSession::progress, &app,
                   [&](int, int, QString) { ++progressCount; });
  QObject::connect(&session, &djapp::ImportSession::finished, &app,
                   [&](int imported, int failed) {
                     gotImported = imported;
                     gotFailed = failed;
                     finished = true;
                     app.quit();
                   });
  // Watchdog: if the import deadlocks, this fires and fails the test instead of
  // hanging CI forever.
  QTimer::singleShot(30'000, &app, [&] {
    timedOut = true;
    app.quit();
  });

  session.start(dbPath, QString::fromStdString(dir.string()));
  app.exec();

  EXPECT_FALSE(timedOut)
      << "import never finished — GUI-thread deadlock regression?";
  EXPECT_TRUE(finished);
  EXPECT_EQ(gotImported, kFiles);
  EXPECT_EQ(gotFailed, 0);
  EXPECT_GT(progressCount, 0);

  {
    Database db(dbPath);
    TrackRepository tracks(db);
    EXPECT_EQ(tracks.count(), kFiles);
  }

  fs::remove_all(dir);
}
