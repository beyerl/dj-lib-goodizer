// djcli command implementations. Drives the real pipeline: import a folder ->
// analyze (all five feature areas) -> apply a profile writing REAL conformed
// output files -> record an audit log. Exposed via runCli() so both the binary
// and the e2e test share it.

#include "CliCommands.h"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "djcore/Version.h"
#include "djcore/analysis/Flags.h"
#include "djcore/audio/AudioDecoder.h"
#include "djcore/audio/AudioError.h"
#include "djcore/db/AnalysisResultRepository.h"
#include "djcore/db/AuditLogRepository.h"
#include "djcore/db/Database.h"
#include "djcore/db/ProfileRepository.h"
#include "djcore/db/Schema.h"
#include "djcore/db/TrackRepository.h"
#include "djcore/jobs/AnalysisBatch.h"
#include "djcore/jobs/JobControl.h"
#include "djcore/processing/ProcessingEngine.h"
#include "djcore/profile/DefaultProfiles.h"

namespace fs = std::filesystem;
using namespace djcore;

namespace {

struct Args {
  std::string command;
  std::vector<std::string> positional;
  std::map<std::string, std::string> flags;

  const std::string& flag(const std::string& key, const std::string& def) const {
    auto it = flags.find(key);
    return it == flags.end() ? def : it->second;
  }
  bool has(const std::string& key) const { return flags.count(key) != 0; }
};

Args parseArgs(int argc, char** argv) {
  Args a;
  if (argc > 1) a.command = argv[1];
  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg.rfind("--", 0) == 0) {
      std::string key = arg.substr(2);
      if (i + 1 < argc && std::string(argv[i + 1]).rfind("--", 0) != 0) {
        a.flags[key] = argv[++i];
      } else {
        a.flags[key] = "";  // boolean flag
      }
    } else {
      a.positional.push_back(arg);
    }
  }
  return a;
}

void printUsage() {
  std::cout << "djcli " << versionString() << " -- DJ library preparation CLI\n\n"
            << "Usage:\n"
            << "  djcli import <folder> --db <db>\n"
            << "  djcli analyze --db <db> [--jobs N]\n"
            << "  djcli list --db <db>\n"
            << "  djcli apply --db <db> --profile <name> --out <dir> "
               "[--no-gain] [--no-trim]\n"
            << "  djcli audit --db <db> [--export <file>]\n"
            << "  djcli profiles --db <db>\n"
            << "  djcli --version | --help\n";
}

std::string jsonEscape(const std::string& s) {
  std::string out;
  for (char c : s) {
    if (c == '"' || c == '\\') out.push_back('\\');
    out.push_back(c);
  }
  return out;
}

std::int64_t nowUnix() { return static_cast<std::int64_t>(std::time(nullptr)); }

// Opens the library DB, applies migrations, and seeds the default profiles.
Database openLibrary(const std::string& path) {
  Database db(path);
  migrate(db);
  ProfileRepository(db).seedDefaultsIfEmpty();
  return db;
}

int cmdImport(const Args& a) {
  if (a.positional.empty() || a.flag("db", "").empty()) {
    std::cerr << "import: need <folder> and --db <db>\n";
    return 2;
  }
  const fs::path folder = a.positional[0];
  Database db = openLibrary(a.flag("db", ""));
  TrackRepository tracks(db);

  int imported = 0, skipped = 0, failed = 0;
  std::error_code ec;
  for (fs::recursive_directory_iterator it(folder, ec), end; it != end;
       it.increment(ec)) {
    if (ec) break;
    if (!it->is_regular_file(ec)) continue;
    const std::string path = it->path().string();
    if (!canDecode(path)) {
      ++skipped;
      continue;
    }
    try {
      Track t;
      t.sourcePath = path;
      t.format = probeFormat(path);
      t.importedAtUnix = nowUnix();
      tracks.insert(t);
      ++imported;
    } catch (const std::exception& e) {
      std::cerr << "  failed: " << path << " (" << e.what() << ")\n";
      ++failed;
    }
  }
  std::cout << "imported " << imported << ", skipped " << skipped << ", failed "
            << failed << " (total tracks: " << tracks.count() << ")\n";
  return failed > 0 ? 1 : 0;
}

int cmdAnalyze(const Args& a) {
  if (a.flag("db", "").empty()) {
    std::cerr << "analyze: need --db <db>\n";
    return 2;
  }
  Database db = openLibrary(a.flag("db", ""));
  TrackRepository tracks(db);
  AnalysisResultRepository results(db);

  const auto all = tracks.all();
  std::vector<std::string> paths;
  paths.reserve(all.size());
  for (const auto& t : all) paths.push_back(t.sourcePath);

  const unsigned jobs =
      a.has("jobs") ? static_cast<unsigned>(std::stoul(a.flag("jobs", "0"))) : 0;

  JobControl control;
  AnalysisBatch batch;
  std::cout << "analyzing " << paths.size() << " track(s)...\n";
  auto items = batch.run(paths, control, {}, jobs);

  int ok = 0, failed = 0;
  for (std::size_t i = 0; i < items.size(); ++i) {
    if (items[i].ok && items[i].result) {
      results.upsert(all[i].id, *items[i].result);
      ++ok;
    } else {
      ++failed;
      if (!items[i].error.empty())
        std::cerr << "  failed: " << items[i].path << " (" << items[i].error << ")\n";
    }
  }
  std::cout << "analyzed " << ok << ", failed " << failed << "\n";
  return failed > 0 ? 1 : 0;
}

int cmdList(const Args& a) {
  if (a.flag("db", "").empty()) {
    std::cerr << "list: need --db <db>\n";
    return 2;
  }
  Database db = openLibrary(a.flag("db", ""));
  TrackRepository tracks(db);
  AnalysisResultRepository results(db);
  for (const auto& t : tracks.all()) {
    std::cout << t.id << "\t" << t.format.container << " " << t.format.sampleRate
              << "Hz/" << t.format.bitDepth << "bit/" << t.format.channels << "ch";
    if (auto r = results.getByTrack(t.id)) {
      std::cout << "\tLUFS=" << r->integratedLufs << " DR=" << r->drValue
                << " crest=" << r->crestFactor;
      if (r->phaseCorrelation) std::cout << " corr=" << *r->phaseCorrelation;
      if (r->stereoWidth) std::cout << " width=" << *r->stereoWidth;
    } else {
      std::cout << "\t(not analyzed)";
    }
    std::cout << "\t" << fs::path(t.sourcePath).filename().string() << "\n";
  }
  return 0;
}

int cmdApply(const Args& a) {
  if (a.flag("db", "").empty() || a.flag("out", "").empty()) {
    std::cerr << "apply: need --db <db> and --out <dir>\n";
    return 2;
  }
  Database db = openLibrary(a.flag("db", ""));
  ProfileRepository profiles(db);
  const std::string profileName = a.flag("profile", "Archival / Lossless");
  auto profileOpt = profiles.getByName(profileName);
  if (!profileOpt) profileOpt = defaultProfileByName(profileName);
  if (!profileOpt) {
    std::cerr << "apply: unknown profile '" << profileName << "'\n";
    return 2;
  }

  const fs::path outDir = a.flag("out", "");
  std::error_code ec;
  fs::create_directories(outDir, ec);

  TrackRepository tracks(db);
  AuditLogRepository audit(db);
  AnalysisResultRepository results(db);
  ProcessingEngine engine(*profileOpt);

  ProcessingOptions options;
  options.applyGain = !a.has("no-gain");
  options.applyTrim = !a.has("no-trim");

  int processed = 0, failed = 0;
  for (const auto& t : tracks.all()) {
    const fs::path outPath =
        outDir / (fs::path(t.sourcePath).stem().string() + "." + profileOpt->container);
    try {
      AnalysisResult analysis;
      if (auto r = results.getByTrack(t.id)) analysis = *r;

      ProcessingChange ch = engine.process(t, analysis, outPath.string(), options);
      tracks.setOutputPath(t.id, outPath.string());

      OperationType op = OperationType::None;
      if (ch.gainApplied)
        op = OperationType::GainNormalize;
      else if (ch.resampled || ch.requantized || ch.containerChanged)
        op = OperationType::ResampleTranscode;
      else if (ch.trimmed)
        op = OperationType::SilenceTrim;

      AuditLogEntry entry;
      entry.trackId = t.id;
      entry.operation = op;
      entry.paramsJson = "{\"profile\":\"" + jsonEscape(profileName) +
                         "\",\"passthrough\":" + (ch.passthrough ? "true" : "false") +
                         ",\"gainDb\":" + std::to_string(ch.gainDb) +
                         ",\"trimLeadMs\":" + std::to_string(ch.trimLeadMs) + "}";
      entry.beforeJson = "{\"container\":\"" + jsonEscape(ch.fromContainer) +
                         "\",\"sampleRate\":" + std::to_string(ch.fromRate) +
                         ",\"bitDepth\":" + std::to_string(ch.fromBits) + "}";
      entry.afterJson = "{\"container\":\"" + jsonEscape(ch.toContainer) +
                        "\",\"sampleRate\":" + std::to_string(ch.toRate) +
                        ",\"bitDepth\":" + std::to_string(ch.toBits) +
                        ",\"durationMs\":" + std::to_string(ch.outDurationMs) + "}";
      entry.timestampUnix = nowUnix();
      audit.insert(entry);

      std::cout << "  " << fs::path(t.sourcePath).filename().string() << " -> "
                << outPath.string() << (ch.passthrough ? "  [passthrough]" : "")
                << "\n";
      ++processed;
    } catch (const std::exception& e) {
      std::cerr << "  failed: " << t.sourcePath << " (" << e.what() << ")\n";
      ++failed;
    }
  }
  std::cout << "processed " << processed << ", failed " << failed << "\n";
  return failed > 0 ? 1 : 0;
}

int cmdAudit(const Args& a) {
  if (a.flag("db", "").empty()) {
    std::cerr << "audit: need --db <db>\n";
    return 2;
  }
  Database db = openLibrary(a.flag("db", ""));
  AuditLogRepository audit(db);
  const auto entries = audit.all();

  const std::string exportPath = a.flag("export", "");
  if (!exportPath.empty()) {
    std::ofstream out(exportPath, std::ios::binary);
    out << "[\n";
    for (std::size_t i = 0; i < entries.size(); ++i) {
      const auto& e = entries[i];
      out << "  {\"id\":" << e.id << ",\"trackId\":" << e.trackId
          << ",\"operation\":\"" << toString(e.operation)
          << "\",\"params\":" << (e.paramsJson.empty() ? "null" : e.paramsJson)
          << ",\"before\":" << (e.beforeJson.empty() ? "null" : e.beforeJson)
          << ",\"after\":" << (e.afterJson.empty() ? "null" : e.afterJson)
          << ",\"timestamp\":" << e.timestampUnix << "}"
          << (i + 1 < entries.size() ? "," : "") << "\n";
    }
    out << "]\n";
    std::cout << "exported " << entries.size() << " audit entries to " << exportPath
              << "\n";
    return 0;
  }

  for (const auto& e : entries) {
    std::cout << e.id << "\ttrack " << e.trackId << "\t" << toString(e.operation)
              << "\t" << e.afterJson << "\n";
  }
  std::cout << "(" << entries.size() << " audit entries)\n";
  return 0;
}

int cmdProfiles(const Args& a) {
  if (!a.flag("db", "").empty()) {
    Database db = openLibrary(a.flag("db", ""));
    ProfileRepository profiles(db);
    for (const auto& p : profiles.all()) {
      std::cout << p.name << "\tcontainer=" << p.container << " sampleRate="
                << (p.sampleRate ? std::to_string(p.sampleRate) : "keep")
                << " bitDepth=" << (p.bitDepth ? std::to_string(p.bitDepth) : "keep")
                << " loudness=" << p.loudnessTargetLufs << "\n";
    }
    return 0;
  }
  for (const auto& p : defaultProfiles()) {
    std::cout << p.name << "\tcontainer=" << p.container << " sampleRate="
              << (p.sampleRate ? std::to_string(p.sampleRate) : "keep")
              << " bitDepth=" << (p.bitDepth ? std::to_string(p.bitDepth) : "keep")
              << "\n";
  }
  return 0;
}

}  // namespace

namespace djcli {

int runCli(int argc, char** argv) {
  Args a = parseArgs(argc, argv);

  if (a.command.empty() || a.command == "--help" || a.command == "-h" ||
      a.command == "help") {
    printUsage();
    return 0;
  }
  if (a.command == "--version" || a.command == "-v" || a.command == "version") {
    std::cout << "djcli " << versionString() << "\n";
    return 0;
  }

  try {
    if (a.command == "import") return cmdImport(a);
    if (a.command == "analyze") return cmdAnalyze(a);
    if (a.command == "list") return cmdList(a);
    if (a.command == "apply") return cmdApply(a);
    if (a.command == "audit") return cmdAudit(a);
    if (a.command == "profiles") return cmdProfiles(a);
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }

  std::cerr << "unknown command: " << a.command << "\n";
  printUsage();
  return 2;
}

}  // namespace djcli
