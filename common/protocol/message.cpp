#include "protocol/message.h"

#include <QJsonDocument>
#include <QJsonValue>

#include "protocol/command_utils.h"

namespace common {

namespace {

QString statusToString(MessageStatus status)
{
    switch (status) {
    case MessageStatus::Success:
        return QStringLiteral("success");
    case MessageStatus::Failure:
        return QStringLiteral("fail");
    case MessageStatus::None:
    default:
        return {};
    }
}

MessageStatus statusFromString(const QString& value)
{
    const QString lowered = value.toLower();
    if (lowered == QStringLiteral("success")) {
        return MessageStatus::Success;
    }
    if (lowered == QStringLiteral("fail") || lowered == QStringLiteral("failure")) {
        return MessageStatus::Failure;
    }
    return MessageStatus::None;
}

} // namespace

Message::Message()
    : Message(Command::Unknown)
{
}

Message::Message(Command command,
                 const QJsonObject& payload,
                 QString requestId,
                 QString sessionToken,
                 MessageStatus status,
                 ErrorCode errorCode,
                 QString statusMessage)
    : command_(command)
    , requestId_(std::move(requestId))
    , sessionToken_(std::move(sessionToken))
    , status_(status)
    , errorCode_(errorCode)
    , statusMessage_(std::move(statusMessage))
    , payload_(payload)
{
}

Command Message::command() const
{
    return command_;
}

const QJsonObject& Message::payload() const
{
    return payload_;
}

void Message::setPayload(const QJsonObject& payload)
{
    payload_ = payload;
}

const QString& Message::requestId() const
{
    return requestId_;
}

void Message::setRequestId(const QString& requestId)
{
    requestId_ = requestId;
}

const QString& Message::sessionToken() const
{
    return sessionToken_;
}

void Message::setSessionToken(const QString& token)
{
    sessionToken_ = token;
}

MessageStatus Message::status() const
{
    return status_;
}

void Message::setStatus(MessageStatus status)
{
    status_ = status;
}

ErrorCode Message::errorCode() const
{
    return errorCode_;
}

void Message::setErrorCode(ErrorCode code)
{
    errorCode_ = code;
}

const QString& Message::statusMessage() const
{
    return statusMessage_;
}

void Message::setStatusMessage(const QString& message)
{
    statusMessage_ = message;
}

bool Message::isSuccess() const
{
    return status_ == MessageStatus::Success;
}

bool Message::isFailure() const
{
    return status_ == MessageStatus::Failure;
}

QByteArray Message::serialize() const
{
    QJsonObject envelope;
    envelope.insert(QStringLiteral("command"), commandToString(command_));
    if (!requestId_.isEmpty()) {
        envelope.insert(QStringLiteral("requestId"), requestId_);
    }
    if (!sessionToken_.isEmpty()) {
        envelope.insert(QStringLiteral("sessionToken"), sessionToken_);
    }
    if (status_ != MessageStatus::None) {
        envelope.insert(QStringLiteral("status"), statusToString(status_));
    }
    if (errorCode_ != ErrorCode::None) {
        envelope.insert(QStringLiteral("errorCode"), errorCodeToString(errorCode_));
    }
    if (!statusMessage_.isEmpty()) {
        envelope.insert(QStringLiteral("message"), statusMessage_);
    }

    envelope.insert(QStringLiteral("payload"), payload_);

    const QJsonDocument doc(envelope);
    return doc.toJson(QJsonDocument::Compact);
}

std::optional<Message> Message::deserialize(const QByteArray& bytes, QString* error)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error) {
            *error = parseError.errorString();
        }
        return std::nullopt;
    }
    if (!doc.isObject()) {
        if (error) {
            *error = QStringLiteral("Root JSON is not an object");
        }
        return std::nullopt;
    }

    const QJsonObject envelope = doc.object();
    const QString commandString = envelope.value(QStringLiteral("command")).toString();
    const Command command = commandFromString(commandString);
    if (command == Command::Unknown) {
        if (error) {
            *error = QStringLiteral("Unknown command: %1").arg(commandString);
        }
        return std::nullopt;
    }

    const QJsonValue payloadValue = envelope.value(QStringLiteral("payload"));
    const QJsonObject payload = payloadValue.isObject() ? payloadValue.toObject() : QJsonObject{};

    Message message(command, payload);
    message.requestId_ = envelope.value(QStringLiteral("requestId")).toString();
    message.sessionToken_ = envelope.value(QStringLiteral("sessionToken")).toString();

    const QString statusString = envelope.value(QStringLiteral("status")).toString();
    if (!statusString.isEmpty()) {
        message.status_ = statusFromString(statusString);
    }

    const QString errorCodeString = envelope.value(QStringLiteral("errorCode")).toString();
    if (!errorCodeString.isEmpty()) {
        message.errorCode_ = errorCodeFromString(errorCodeString);
        if (message.errorCode_ != ErrorCode::None && message.status_ == MessageStatus::None) {
            message.status_ = MessageStatus::Failure;
        }
    }

    const QString statusMessage = envelope.value(QStringLiteral("message")).toString();
    if (!statusMessage.isEmpty()) {
        message.statusMessage_ = statusMessage;
    }

    if (message.status_ == MessageStatus::None && payload.contains(QStringLiteral("success"))) {
        if (payload.value(QStringLiteral("success")).toBool(false)) {
            message.status_ = MessageStatus::Success;
        } else {
            message.status_ = MessageStatus::Failure;
        }
    }

    return message;
}

Message Message::makeSuccess(Command command,
                             const QJsonObject& payload,
                             QString requestId,
                             QString sessionToken,
                             QString statusMessage)
{
    Message message(command, payload, std::move(requestId), std::move(sessionToken));
    message.status_ = MessageStatus::Success;
    message.errorCode_ = ErrorCode::None;
    message.statusMessage_ = std::move(statusMessage);
    return message;
}

Message Message::makeFailure(Command command,
                             ErrorCode errorCode,
                             QString statusMessage,
                             const QJsonObject& payload,
                             QString requestId,
                             QString sessionToken)
{
    Message message(command, payload, std::move(requestId), std::move(sessionToken));
    message.status_ = MessageStatus::Failure;
    message.errorCode_ = errorCode;
    message.statusMessage_ = std::move(statusMessage);
    return message;
}

} // namespace common
