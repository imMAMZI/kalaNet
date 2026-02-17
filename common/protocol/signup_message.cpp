#include "protocol/signup_message.h"

#include <QJsonObject>
#include <QString>

namespace common {

    Message SignupMessage::createRequest(const QString& fullName,
                                      const QString& username,
                                      const QString& phone,
                                      const QString& email,
                                      const QString& password)
    {
        QJsonObject payload{
            {"fullName", fullName},
            {"username", username},
            {"phone", phone},
            {"email", email},
            {"password", password}
        };

        return Message(Command::Signup, payload);
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

}
