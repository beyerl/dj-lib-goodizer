#pragma once

#include <QAbstractTableModel>
#include <optional>
#include <vector>

#include "djcore/model/AnalysisResult.h"
#include "djcore/model/TargetProfile.h"
#include "djcore/model/Track.h"

namespace djcore {
class Database;
}

namespace djapp {

struct LibraryRow {
  djcore::Track track;
  std::optional<djcore::AnalysisResult> analysis;
};

// Table model for the Library View. Columns: file, format, sample rate,
// channels, loudness, dynamic range, mono flag, width flag. Loudness/DR/mono/
// width cells are coloured by the three-state flag model against the active
// profile. Sorting via a QSortFilterProxyModel in the view.
class LibraryModel : public QAbstractTableModel {
  Q_OBJECT
 public:
  enum Column {
    File = 0,
    Format,
    SampleRate,
    Channels,
    Loudness,
    DynamicRange,
    Mono,
    Width,
    ColumnCount
  };

  explicit LibraryModel(QObject* parent = nullptr);

  void setProfile(const djcore::TargetProfile& profile);
  void reload(djcore::Database& db);

  int rowCount(const QModelIndex& parent = {}) const override;
  int columnCount(const QModelIndex& parent = {}) const override;
  QVariant data(const QModelIndex& index, int role) const override;
  QVariant headerData(int section, Qt::Orientation o, int role) const override;

  const LibraryRow* rowAt(int row) const;
  int trackCount() const { return static_cast<int>(rows_.size()); }
  const std::vector<LibraryRow>& rows() const { return rows_; }
  const djcore::TargetProfile& profile() const { return profile_; }

 private:
  std::vector<LibraryRow> rows_;
  djcore::TargetProfile profile_;
};

}  // namespace djapp
