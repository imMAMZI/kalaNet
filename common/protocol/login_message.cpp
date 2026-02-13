#include "protocol/login_message.h"

#include <QJsonObject>
#include <QString>

namespace common {

    Message LoginMessage::createRequest(const std::string& username,
                                        const std::string& password,
                                        const QString& requestId,
                                        const QString& captchaToken)
    {
        QJsonObject payload;
        payload.insert(QStringLiteral("username"), QString::fromStdString(username));
        payload.insert(QStringLiteral("password"), QString::fromStdString(password));
        if (!captchaToken.isEmpty()) {
            payload.insert(QStringLiteral("captchaToken"), captchaToken);
        }

        return Message(Command::Login, payload, requestId);
    }

    Message LoginMessage::createSuccessResponse(const QJsonObject& userPayload,
                                                const QString& sessionToken,
                                                const QString& requestId,
                                                const QString& statusMessage)
    {
        QJsonObject payload;
        payload.insert(QStringLiteral("success"), true);
        payload.insert(QStringLiteral("user"), userPayload);
        payload.insert(QStringLiteral("sessionToken"), sessionToken);

        return Message::makeSuccess(Command::LoginResult, payload, requestId, sessionToken, statusMessage);
    }

    Message LoginMessage::createFailureResponse(ErrorCode errorCode,
                                                const QString& reason,
                                                const QString& requestId,
                                                const QJsonObject& payload)
    {
        QJsonObject responsePayload = payload;
        responsePayload.insert(QStringLiteral("success"), false);
        responsePayload.insert(QStringLiteral("reason"), reason);

        return Message::makeFailure(Command::LoginResult, errorCode, reason, responsePayload, requestId);
    }

} // namespace common
