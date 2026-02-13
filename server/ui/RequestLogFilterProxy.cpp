#include "RequestLogFilterProxy.h"

RequestLogFilterProxy::RequestLogFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

void RequestLogFilterProxy::setCommandFilter(const QString& command)
{
    commandFilter_ = command;
    invalidateFilter();
}

void RequestLogFilterProxy::setStatusFilter(const QString& status)
{
    statusFilter_ = status;
    invalidateFilter();
}

void RequestLogFilterProxy::setTextFilter(const QString& text)
{
    textFilter_ = text;
    invalidateFilter();
}

bool RequestLogFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    const auto indexCommand = sourceModel()->index(sourceRow, 3, sourceParent);
    const auto indexStatus  = sourceModel()->index(sourceRow, 4, sourceParent);
    const auto indexError   = sourceModel()->index(sourceRow, 5, sourceParent);
    const auto indexMessage = sourceModel()->index(sourceRow, 6, sourceParent);
    const auto indexUser    = sourceModel()->index(sourceRow, 2, sourceParent);
    const auto indexRemote  = sourceModel()->index(sourceRow, 1, sourceParent);

    const auto commandValue = sourceModel()->data(indexCommand).toString();
    const auto statusValue  = sourceModel()->data(indexStatus).toString();
    const auto errorValue   = sourceModel()->data(indexError).toString();
    const auto messageValue = sourceModel()->data(indexMessage).toString();
    const auto userValue    = sourceModel()->data(indexUser).toString();
    const auto remoteValue  = sourceModel()->data(indexRemote).toString();

    if (commandFilter_ != QObject::tr("All Commands") && commandValue != commandFilter_) {
        return false;
    }
    if (statusFilter_ != QObject::tr("All Statuses") && statusValue != statusFilter_) {
        return false;
    }
    if (!textFilter_.isEmpty()) {
        const auto haystack = QStringList{
            commandValue, statusValue, errorValue, messageValue, userValue, remoteValue
        }.join(' ').toLower();

        if (!haystack.contains(textFilter_.toLower())) {
            return false;
        }
    }
    return true;
}
