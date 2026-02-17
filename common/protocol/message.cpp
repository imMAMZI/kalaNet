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

}

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
    return QJsonDocument(toJson()).toJson(QJsonDocument::Compact);
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

    return fromJson(doc.object(), error);
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
    QJsonObject enrichedPayload = payload;
    if (!enrichedPayload.contains(QStringLiteral("statusCode"))) {
        enrichedPayload.insert(QStringLiteral("statusCode"), errorCodeToStatusCode(errorCode));
    }

    Message message(command, enrichedPayload, std::move(requestId), std::move(sessionToken));
    message.status_ = MessageStatus::Failure;
    message.errorCode_ = errorCode;
    message.statusMessage_ = std::move(statusMessage);
    return message;
}
QJsonObject Message::toJson() const
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
    return envelope;
}

std::optional<Message> Message::fromJson(const QJsonObject& envelope, QString* error)
{
    const QJsonValue commandValue = envelope.value(QStringLiteral("command"));
    if (!commandValue.isString()) {
        if (error) {
            *error = QStringLiteral("Missing or invalid 'command' field");
        }
        return std::nullopt;
    }

    const QString commandStr = commandValue.toString();
    const Command parsedCommand = commandFromString(commandStr);
    if (commandToString(parsedCommand).compare(commandStr, Qt::CaseInsensitive) != 0) {
        if (error) {
            *error = QStringLiteral("Unknown command: %1").arg(commandStr);
        }
        return std::nullopt;
    }

    Message message(parsedCommand);

    if (const QJsonValue requestIdValue = envelope.value(QStringLiteral("requestId"));
        requestIdValue.isString()) {
        message.requestId_ = requestIdValue.toString();
    }

    if (const QJsonValue sessionTokenValue = envelope.value(QStringLiteral("sessionToken"));
        sessionTokenValue.isString()) {
        message.sessionToken_ = sessionTokenValue.toString();
    }

    message.status_ = MessageStatus::None;
    if (const QJsonValue statusValue = envelope.value(QStringLiteral("status"));
        !statusValue.isUndefined()) {
        if (!statusValue.isString()) {
            if (error) {
                *error = QStringLiteral("'status' must be a string");
            }
            return std::nullopt;
        }

        const QString statusStr = statusValue.toString();
        const MessageStatus parsedStatus = statusFromString(statusStr);
        if (statusToString(parsedStatus).compare(statusStr, Qt::CaseInsensitive) != 0) {
            if (error) {
                *error = QStringLiteral("Unknown status: %1").arg(statusStr);
            }
            return std::nullopt;
        }

        message.status_ = parsedStatus;
    }

    message.errorCode_ = ErrorCode::None;
    if (const QJsonValue errorCodeValue = envelope.value(QStringLiteral("errorCode"));
        !errorCodeValue.isUndefined()) {
        if (!errorCodeValue.isString()) {
            if (error) {
                *error = QStringLiteral("'errorCode' must be a string");
            }
            return std::nullopt;
        }

        const QString errorCodeStr = errorCodeValue.toString();
        const ErrorCode parsedErrorCode = errorCodeFromString(errorCodeStr);
        if (errorCodeToString(parsedErrorCode).compare(errorCodeStr, Qt::CaseInsensitive) != 0) {
            if (error) {
                *error = QStringLiteral("Unknown error code: %1").arg(errorCodeStr);
            }
            return std::nullopt;
        }

        message.errorCode_ = parsedErrorCode;
    }

    if (const QJsonValue statusMessageValue = envelope.value(QStringLiteral("message"));
        statusMessageValue.isString()) {
        message.statusMessage_ = statusMessageValue.toString();
    }

    const QJsonValue payloadValue = envelope.value(QStringLiteral("payload"));
    if (payloadValue.isUndefined()) {
        message.payload_ = QJsonObject{};
    } else if (!payloadValue.isObject()) {
        if (error) {
            *error = QStringLiteral("'payload' must be a JSON object");
        }
        return std::nullopt;
    } else {
        message.payload_ = payloadValue.toObject();
    }

    return message;
}

}
