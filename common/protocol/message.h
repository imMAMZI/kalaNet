#ifndef COMMON_PROTOCOL_MESSAGE_H
#define COMMON_PROTOCOL_MESSAGE_H

#include <optional>

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include "protocol/commands.h"
#include "protocol/error_codes.h"

namespace common {

enum class MessageStatus {
    None = 0,
    Success,
    Failure
};

// ... بخش‌های بالایی فایل بدون تغییر ...

class Message {
public:
    Message();
    Message(Command command,
            const QJsonObject& payload = {},
            QString requestId = {},
            QString sessionToken = {},
            MessageStatus status = MessageStatus::None,
            ErrorCode errorCode = ErrorCode::None,
            QString statusMessage = {});

    Command command() const;

    const QJsonObject& payload() const;
    void setPayload(const QJsonObject& payload);

    const QString& requestId() const;
    void setRequestId(const QString& requestId);

    const QString& sessionToken() const;
    void setSessionToken(const QString& token);

    MessageStatus status() const;
    void setStatus(MessageStatus status);

    ErrorCode errorCode() const;
    void setErrorCode(ErrorCode code);

    const QString& statusMessage() const;
    void setStatusMessage(const QString& message);

    bool isSuccess() const;
    bool isFailure() const;

    QByteArray serialize() const;
    static std::optional<Message> deserialize(const QByteArray& bytes, QString* error = nullptr);

    QJsonObject toJson() const;
    QJsonObject json() const { return toJson(); }
    static std::optional<Message> fromJson(const QJsonObject& object, QString* error = nullptr);

    static Message makeSuccess(Command command,
                               const QJsonObject& payload = {},
                               QString requestId = {},
                               QString sessionToken = {},
                               QString statusMessage = {});
    static Message makeFailure(Command command,
                               ErrorCode errorCode,
                               QString statusMessage,
                               const QJsonObject& payload = {},
                               QString requestId = {},
                               QString sessionToken = {});

private:
    Command command_;
    QString requestId_;
    QString sessionToken_;
    MessageStatus status_;
    ErrorCode errorCode_;
    QString statusMessage_;
    QJsonObject payload_;
};

// ... بقیه فایل بدون تغییر ...

} // namespace common

#endif // COMMON_PROTOCOL_MESSAGE_H
