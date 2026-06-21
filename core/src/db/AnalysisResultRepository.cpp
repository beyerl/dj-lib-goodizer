#include "djcore/db/AnalysisResultRepository.h"

#include "djcore/db/Database.h"

namespace djcore {
namespace {

void bindOptional(Statement& s, int i, const std::optional<double>& v) {
  if (v.has_value()) s.bind(i, *v); else s.bindNull(i);
}

std::optional<double> readOptional(Statement& s, int c) {
  if (s.columnIsNull(c)) return std::nullopt;
  return s.columnDouble(c);
}

}  // namespace

void AnalysisResultRepository::upsert(std::int64_t trackId, const AnalysisResult& r) {
  Statement s = db_.prepare(
      "INSERT INTO analysis_result (track_id, leading_silence_ms, "
      "trailing_silence_ms, silence_ambiguous, integrated_lufs, true_peak_dbtp, "
      "crest_factor, dr_value, phase_correlation, mono_folddown_db, stereo_width, "
      "lr_balance_db, analyzer_version, analyzed_at) "
      "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?) "
      "ON CONFLICT(track_id) DO UPDATE SET "
      "leading_silence_ms=excluded.leading_silence_ms, "
      "trailing_silence_ms=excluded.trailing_silence_ms, "
      "silence_ambiguous=excluded.silence_ambiguous, "
      "integrated_lufs=excluded.integrated_lufs, "
      "true_peak_dbtp=excluded.true_peak_dbtp, "
      "crest_factor=excluded.crest_factor, dr_value=excluded.dr_value, "
      "phase_correlation=excluded.phase_correlation, "
      "mono_folddown_db=excluded.mono_folddown_db, "
      "stereo_width=excluded.stereo_width, lr_balance_db=excluded.lr_balance_db, "
      "analyzer_version=excluded.analyzer_version, analyzed_at=excluded.analyzed_at;");
  s.bind(1, trackId);
  s.bind(2, r.leadingSilenceMs);
  s.bind(3, r.trailingSilenceMs);
  s.bind(4, static_cast<std::int64_t>(r.silenceAmbiguous ? 1 : 0));
  s.bind(5, r.integratedLufs);
  s.bind(6, r.truePeakDbtp);
  s.bind(7, r.crestFactor);
  s.bind(8, r.drValue);
  bindOptional(s, 9, r.phaseCorrelation);
  bindOptional(s, 10, r.monoFolddownDeltaDb);
  bindOptional(s, 11, r.stereoWidth);
  bindOptional(s, 12, r.lrBalanceDb);
  s.bind(13, static_cast<std::int64_t>(r.analyzerVersion));
  s.bind(14, r.analyzedAtUnix);
  s.step();
}

std::optional<AnalysisResult> AnalysisResultRepository::getByTrack(std::int64_t trackId) {
  Statement s = db_.prepare(
      "SELECT leading_silence_ms, trailing_silence_ms, silence_ambiguous, "
      "integrated_lufs, true_peak_dbtp, crest_factor, dr_value, "
      "phase_correlation, mono_folddown_db, stereo_width, lr_balance_db, "
      "analyzer_version, analyzed_at FROM analysis_result WHERE track_id=?;");
  s.bind(1, trackId);
  if (!s.step()) return std::nullopt;

  AnalysisResult r;
  r.leadingSilenceMs = s.columnInt64(0);
  r.trailingSilenceMs = s.columnInt64(1);
  r.silenceAmbiguous = s.columnInt64(2) != 0;
  r.integratedLufs = s.columnDouble(3);
  r.truePeakDbtp = s.columnDouble(4);
  r.crestFactor = s.columnDouble(5);
  r.drValue = s.columnDouble(6);
  r.phaseCorrelation = readOptional(s, 7);
  r.monoFolddownDeltaDb = readOptional(s, 8);
  r.stereoWidth = readOptional(s, 9);
  r.lrBalanceDb = readOptional(s, 10);
  r.analyzerVersion = static_cast<int>(s.columnInt64(11));
  r.analyzedAtUnix = s.columnInt64(12);
  return r;
}

}  // namespace djcore
