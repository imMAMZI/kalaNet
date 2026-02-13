// server/ui/requestlogmodel.cpp
#include "requestlogmodel.h"

RequestLogModel::RequestLogModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int RequestLogModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : entries_.size();
}

int RequestLogModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : 7;
}

QVariant RequestLogModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) {
        return {};
    }

    const auto& entry = entries_.at(index.row());
    switch (index.column()) {
        case 0: return entry.timestamp;
        case 1: return entry.remoteAddress;
        case 2: return entry.username.isEmpty() ? entry.sessionToken : entry.username;
        case 3: return entry.command;
        case 4: return entry.statusCode;
        case 5: return entry.errorCode;
        case 6: return entry.message;
        default: return {};
    }
}

QVariant RequestLogModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return columnLabel(section);
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

void RequestLogModel::appendLog(RequestLogEntry entry) {
    beginInsertRows({}, entries_.size(), entries_.size());
    entries_.push_back(std::move(entry));
    endInsertRows();
    emit logCountChanged(entries_.size());
}

void RequestLogModel::clear() {
    beginResetModel();
    entries_.clear();
    endResetModel();
    emit logCountChanged(0);
}

RequestLogEntry RequestLogModel::entryAt(int row) const {
    return entries_.value(row);
}

QString RequestLogModel::columnLabel(int section) const {
    switch (section) {
        case 0: return tr("Timestamp");
        case 1: return tr("Remote");
        case 2: return tr("User / Session");
        case 3: return tr("Command");
        case 4: return tr("Status");
        case 5: return tr("Error");
        case 6: return tr("Message");
        default: return {};
    }
}
