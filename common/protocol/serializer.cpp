#include "serializer.h"
#include <QJsonDocument>
#include <QJsonParseError>

using namespace common;

QByteArray Serializer::serialize(const Message& message)
{
    QJsonDocument doc(message.toJson());
    return doc.toJson(QJsonDocument::Compact);
}

Message Serializer::deserialize(const QByteArray& data)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return Message(Command::Error,
                       QJsonObject{{"message", "Invalid JSON"}});
    }

    return Message::fromJson(doc.object());
}
