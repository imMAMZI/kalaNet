// common/protocol/serializer.cpp
#include "serializer.h"

#include <QJsonObject>
#include <QString>

#include "protocol/error_codes.h"

using namespace common;

QByteArray Serializer::serialize(const Message& message)
{
    return message.serialize();
}

Message Serializer::deserialize(const QByteArray& data)
{
    QString parseError;
    auto maybeMessage = Message::deserialize(data, &parseError);
    if (maybeMessage.has_value()) {
        return *maybeMessage;
    }

    QJsonObject payload;
    if (!parseError.isEmpty()) {
        payload.insert(QStringLiteral("message"), parseError);
    }

    Message errorMessage(
        Command::Error,
        payload,
        {},
        {},
        MessageStatus::Failure,
        ErrorCode::InvalidPayload,
        parseError.isEmpty()
            ? QStringLiteral("Invalid message payload")
            : parseError
    );

    return errorMessage;
}
