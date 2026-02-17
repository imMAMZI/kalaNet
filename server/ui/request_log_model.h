//
// Created by hosse on 2/13/2026.
//

#ifndef KALANET_REQUESTLOGMODEL_H
#define KALANET_REQUESTLOGMODEL_H

#include <QAbstractTableModel>
#include <QVector>

struct RequestLogEntry {
    QString timestamp;
    QString remoteAddress;
    QString sessionToken;
    QString username;
    QString command;
    int statusCode;
    QString errorCode;
    QString message;
};

class RequestLogModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit RequestLogModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    void appendLog(RequestLogEntry entry);
    void clear();
    RequestLogEntry entryAt(int row) const;

    signals:
        void logCountChanged(int count);

private:
    QVector<RequestLogEntry> entries_;
    QString columnLabel(int section) const;
};

#endif //KALANET_REQUESTLOGMODEL_H