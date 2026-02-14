#ifndef KALANET_AUDIT_LOGGER_H
#define KALANET_AUDIT_LOGGER_H

#include <QJsonObject>
#include <QString>

class AuditLogger
{
public:
    static void log(const QString& event,
                    const QString& outcome,
                    const QJsonObject& details = QJsonObject{});
};

#endif // KALANET_AUDIT_LOGGER_H
