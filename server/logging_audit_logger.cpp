#include "logging_audit_logger.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(kalanetAudit, "kalanet.audit")

void AuditLogger::log(const QString& event,
                      const QString& outcome,
                      const QJsonObject& details)
{
    QJsonObject payload = details;
    payload.insert(QStringLiteral("event"), event);
    payload.insert(QStringLiteral("outcome"), outcome);
    payload.insert(QStringLiteral("timestamp"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    qCInfo(kalanetAudit).noquote() << QJsonDocument(payload).toJson(QJsonDocument::Compact);
}
