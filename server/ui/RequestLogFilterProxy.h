//
// Created by hosse on 2/13/2026.
//

#ifndef KALANET_REQUESTLOGFILTERPROXY_H
#define KALANET_REQUESTLOGFILTERPROXY_H
#include <QSortFilterProxyModel>
#include <QSortFilterProxyModel>

class RequestLogFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit RequestLogFilterProxy(QObject* parent = nullptr);

    void setCommandFilter(const QString& command);
    void setStatusFilter(const QString& status);
    void setTextFilter(const QString& text);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    QString commandFilter_;
    QString statusFilter_;
    QString textFilter_;
};

#endif //KALANET_REQUESTLOGFILTERPROXY_H