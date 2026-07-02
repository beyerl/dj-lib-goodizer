// End-to-end test of the djcli pipeline: import a folder of real WAVs, apply a
// profile that writes REAL conformed output files, and record an audit log.
// Drives the same runCli() the binary uses, in-process (no shell quoting).

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "CliCommands.h"
#include "djcore/audio/WavIo.h"
#include "djcore/db/AuditLogRepository.h"
#include "djcore/db/Database.h"
#include "djcore/db/Schema.h"
#include "djcore/db/TrackRepository.h"

namespace fs = std::filesystem;
using namespace djcore;

namespace {

int run(std::vector<std::string> args) {
  std::vector<char*> argv;
  argv.push_back(const_cast<char*>("djcli"));
  for (auto& s : args) argv.push_back(const_cast<char*>(s.c_str()));
  return djcli::runCli(static_cast<int>(argv.size()), argv.data());
}

std::vector<unsigned char> readBytes(const fs::path& p) {
  std::ifstream in(p, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

}  // namespace

TEST(CliE2E, ImportApplyAuditWritesRealConformedOutput) {
  const auto root =
      fs::temp_directory_path() /
      ("dje2e_" + std::to_string(static_cast<long long>(::time(nullptr))));
  const auto inDir = root / "in";
  const auto outDir = root / "out";
  fs::create_directories(inDir);
  const std::string db = (root / "lib.db").string();

  // Two source files: 16-bit stereo @44.1k and 24-bit mono @48k.
  PcmBuffer a(2, 44100, 441);
  for (std::size_t i = 0; i < 441; ++i) {
    a.channel(0)[i] = 0.3f;
    a.channel(1)[i] = -0.3f;
  }
  PcmBuffer b(1, 48000, 480);
  for (std::size_t i = 0; i < 480; ++i) b.channel(0)[i] = 0.1f;

  const std::string srcA = (inDir / "a.wav").string();
  const std::string srcB = (inDir / "b.wav").string();
  writeWav(srcA, a, 16, false);
  writeWav(srcB, b, 24, false);
  const auto beforeA = readBytes(srcA);
  const auto beforeB = readBytes(srcB);

  ASSERT_EQ(run({"import", inDir.string(), "--db", db}), 0);
  ASSERT_EQ(run({"apply", "--db", db, "--profile", "Archival / Lossless", "--out",
                 outDir.string()}),
            0);

  // Real output files exist and conform to source format.
  const auto outA = outDir / "a.wav";
  const auto outB = outDir / "b.wav";
  ASSERT_TRUE(fs::exists(outA));
  ASSERT_TRUE(fs::exists(outB));

  FormatInfo ia;
  PcmBuffer ra = readWav(outA.string(), &ia);
  EXPECT_EQ(ia.sampleRate, 44100);
  EXPECT_EQ(ia.channels, 2);
  EXPECT_EQ(ia.bitDepth, 16);
  ASSERT_EQ(ra.frames(), 441u);
  for (std::size_t i = 0; i < 441; ++i) {
    EXPECT_NEAR(ra.channel(0)[i], 0.3f, 1e-4f);
    EXPECT_NEAR(ra.channel(1)[i], -0.3f, 1e-4f);
  }

  FormatInfo ib;
  PcmBuffer rb = readWav(outB.string(), &ib);
  EXPECT_EQ(ib.sampleRate, 48000);
  EXPECT_EQ(ib.channels, 1);
  EXPECT_EQ(ib.bitDepth, 24);
  EXPECT_EQ(rb.frames(), 480u);

  // Sources are never modified (non-destructive).
  EXPECT_EQ(readBytes(srcA), beforeA);
  EXPECT_EQ(readBytes(srcB), beforeB);

  // The DB has both tracks and an audit entry per processed track.
  {
    Database d(db);
    migrate(d);
    TrackRepository tr(d);
    AuditLogRepository ar(d);
    EXPECT_EQ(tr.count(), 2);
    EXPECT_EQ(ar.count(), 2);
  }

  // Audit export writes a non-empty file.
  const std::string exportPath = (root / "audit.json").string();
  ASSERT_EQ(run({"audit", "--db", db, "--export", exportPath}), 0);
  ASSERT_TRUE(fs::exists(exportPath));
  EXPECT_GT(fs::file_size(exportPath), 2u);

  std::error_code ec;
  fs::remove_all(root, ec);
}
