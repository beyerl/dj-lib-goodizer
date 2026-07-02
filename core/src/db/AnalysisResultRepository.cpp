#include "djcore/db/AnalysisResultRepository.h"

#include <optional>

#include "djcore/db/Database.h"

namespace djcore {
namespace {

void bindOpt(Statement& st, int i, const std::optional<double>& v) {
  if (v)
    st.bind(i, *v);
  else
    st.bindNull(i);
}

std::optional<double> readOpt(Statement& st, int col) {
  if (st.columnIsNull(col)) return std::nullopt;
  return st.columnDouble(col);
}

}  // namespace

void AnalysisResultRepository::upsert(std::int64_t trackId,
                                      const AnalysisResult& r) {
  Statement st = db_.prepare(
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
      "true_peak_dbtp=excluded.true_peak_dbtp, crest_factor=excluded.crest_factor, "
      "dr_value=excluded.dr_value, phase_correlation=excluded.phase_correlation, "
      "mono_folddown_db=excluded.mono_folddown_db, "
      "stereo_width=excluded.stereo_width, lr_balance_db=excluded.lr_balance_db, "
      "analyzer_version=excluded.analyzer_version, analyzed_at=excluded.analyzed_at;");
  st.bind(1, trackId);
  st.bind(2, r.leadingSilenceMs);
  st.bind(3, r.trailingSilenceMs);
  st.bind(4, static_cast<std::int64_t>(r.silenceAmbiguous ? 1 : 0));
  st.bind(5, r.integratedLufs);
  st.bind(6, r.truePeakDbtp);
  st.bind(7, r.crestFactor);
  st.bind(8, r.drValue);
  bindOpt(st, 9, r.phaseCorrelation);
  bindOpt(st, 10, r.monoFolddownDeltaDb);
  bindOpt(st, 11, r.stereoWidth);
  bindOpt(st, 12, r.lrBalanceDb);
  st.bind(13, static_cast<std::int64_t>(r.analyzerVersion));
  st.bind(14, r.analyzedAtUnix);
  st.step();
}

std::optional<AnalysisResult> AnalysisResultRepository::getByTrack(
    std::int64_t trackId) {
  Statement st = db_.prepare(
      "SELECT leading_silence_ms, trailing_silence_ms, silence_ambiguous, "
      "integrated_lufs, true_peak_dbtp, crest_factor, dr_value, phase_correlation, "
      "mono_folddown_db, stereo_width, lr_balance_db, analyzer_version, analyzed_at "
      "FROM analysis_result WHERE track_id=?;");
  st.bind(1, trackId);
  if (!st.step()) return std::nullopt;

  AnalysisResult r;
  r.leadingSilenceMs = st.columnInt64(0);
  r.trailingSilenceMs = st.columnInt64(1);
  r.silenceAmbiguous = st.columnInt64(2) != 0;
  r.integratedLufs = st.columnDouble(3);
  r.truePeakDbtp = st.columnDouble(4);
  r.crestFactor = st.columnDouble(5);
  r.drValue = st.columnDouble(6);
  r.phaseCorrelation = readOpt(st, 7);
  r.monoFolddownDeltaDb = readOpt(st, 8);
  r.stereoWidth = readOpt(st, 9);
  r.lrBalanceDb = readOpt(st, 10);
  r.analyzerVersion = static_cast<int>(st.columnInt64(11));
  r.analyzedAtUnix = st.columnInt64(12);
  return r;
}

}  // namespace djcore
