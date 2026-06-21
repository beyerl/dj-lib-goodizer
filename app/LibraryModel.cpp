#include "LibraryModel.h"

#include <QBrush>
#include <QColor>
#include <QFileInfo>

#include "djcore/analysis/Flags.h"
#include "djcore/db/AnalysisResultRepository.h"
#include "djcore/db/Database.h"
#include "djcore/db/TrackRepository.h"

namespace djapp {
namespace {

QColor flagColor(djcore::FlagState s) {
  switch (s) {
    case djcore::FlagState::Ok: return QColor(0xE6, 0xF4, 0xEA);            // green tint
    case djcore::FlagState::Review: return QColor(0xFF, 0xF4, 0xCE);       // amber tint
    case djcore::FlagState::OutsideTarget: return QColor(0xFA, 0xE3, 0xE3);// red tint
  }
  return {};
}

}  // namespace

LibraryModel::LibraryModel(QObject* parent) : QAbstractTableModel(parent) {}

void LibraryModel::setProfile(const djcore::TargetProfile& profile) {
  profile_ = profile;
  if (!rows_.empty()) {
    emit dataChanged(index(0, 0), index(rowCount() - 1, ColumnCount - 1));
  }
}

void LibraryModel::reload(djcore::Database& db) {
  beginResetModel();
  rows_.clear();
  djcore::TrackRepository tracks(db);
  djcore::AnalysisResultRepository results(db);
  for (auto& t : tracks.all()) {
    LibraryRow row;
    row.analysis = results.getByTrack(t.id);
    row.track = std::move(t);
    rows_.push_back(std::move(row));
  }
  endResetModel();
}

int LibraryModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

int LibraryModel::columnCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : ColumnCount;
}

const LibraryRow* LibraryModel::rowAt(int row) const {
  if (row < 0 || row >= static_cast<int>(rows_.size())) return nullptr;
  return &rows_[static_cast<std::size_t>(row)];
}

QVariant LibraryModel::data(const QModelIndex& index, int role) const {
  if (!index.isValid()) return {};
  const LibraryRow& r = rows_[static_cast<std::size_t>(index.row())];
  const auto& a = r.analysis;

  if (role == Qt::DisplayRole) {
    switch (index.column()) {
      case File:
        return QFileInfo(QString::fromStdString(r.track.sourcePath)).fileName();
      case Format:
        return QString::fromStdString(r.track.format.container + " / " +
                                      r.track.format.codec);
      case SampleRate:
        return r.track.format.sampleRate;
      case Channels:
        return r.track.format.channels;
      case Loudness:
        return a ? QString::number(a->integratedLufs, 'f', 1) + " LUFS" : QStringLiteral("—");
      case DynamicRange:
        return a ? QString::number(a->drValue, 'f', 1) + " dB" : QStringLiteral("—");
      case Mono:
        return a ? djcore::flagLabel(djcore::monoFlag(*a, profile_)) : "—";
      case Width:
        return a ? djcore::flagLabel(djcore::widthFlag(*a, profile_)) : "—";
    }
  }

  if (role == Qt::BackgroundRole && a) {
    switch (index.column()) {
      case Loudness: return QBrush(flagColor(djcore::loudnessFlag(*a, profile_)));
      case DynamicRange: return QBrush(flagColor(djcore::dynamicRangeFlag(*a, profile_)));
      case Mono: return QBrush(flagColor(djcore::monoFlag(*a, profile_)));
      case Width: return QBrush(flagColor(djcore::widthFlag(*a, profile_)));
    }
  }

  // Numeric sort keys for the proxy model.
  if (role == Qt::UserRole && a) {
    switch (index.column()) {
      case Loudness: return a->integratedLufs;
      case DynamicRange: return a->drValue;
      default: break;
    }
  }
  return {};
}

QVariant LibraryModel::headerData(int section, Qt::Orientation o, int role) const {
  if (role != Qt::DisplayRole || o != Qt::Horizontal) return {};
  switch (section) {
    case File: return QStringLiteral("File");
    case Format: return QStringLiteral("Format");
    case SampleRate: return QStringLiteral("Rate");
    case Channels: return QStringLiteral("Ch");
    case Loudness: return QStringLiteral("Loudness");
    case DynamicRange: return QStringLiteral("Dynamic Range");
    case Mono: return QStringLiteral("Mono");
    case Width: return QStringLiteral("Width");
  }
  return {};
}

}  // namespace djapp
