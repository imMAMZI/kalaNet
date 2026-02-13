#include "protocol/signup_message.h"

#include <QJsonObject>
#include <QString>

namespace common {

    Message SignupMessage::createRequest(const std::string& username,
                                         const std::string& password,
                                         const std::string& email,
                                         const QString& requestId,
                                         const QJsonObject& profile)
    {
        QJsonObject payload;
        payload.insert(QStringLiteral("username"), QString::fromStdString(username));
        payload.insert(QStringLiteral("password"), QString::fromStdString(password));
        payload.insert(QStringLiteral("email"), QString::fromStdString(email));
        if (!profile.isEmpty()) {
            payload.insert(QStringLiteral("profile"), profile);
        }

        return Message(Command::Signup, payload, requestId);
    }

    Message SignupMessage::createSuccessResponse(const QJsonObject& userPayload,
                                                 const QString& requestId,
                                                 const QString& statusMessage)
    {
        QJsonObject payload;
        payload.insert(QStringLiteral("success"), true);
        payload.insert(QStringLiteral("user"), userPayload);

        return Message::makeSuccess(Command::SignupResult, payload, requestId, {}, statusMessage);
    }

    Message SignupMessage::createFailureResponse(ErrorCode errorCode,
                                                 const QString& reason,
                                                 const QString& requestId,
                                                 const QJsonObject& payload)
    {
        QJsonObject responsePayload = payload;
        responsePayload.insert(QStringLiteral("success"), false);
        responsePayload.insert(QStringLiteral("reason"), reason);

        return Message::makeFailure(Command::SignupResult, errorCode, reason, responsePayload, requestId);
    }

} // namespace common
