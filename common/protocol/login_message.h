#ifndef COMMON_PROTOCOL_LOGIN_MESSAGE_H
#define COMMON_PROTOCOL_LOGIN_MESSAGE_H

#include <string>

#include <QJsonObject>
#include <QString>

#include "protocol/error_codes.h"
#include "protocol/message.h"

namespace common {

    class LoginMessage {
    public:
        static Message createRequest(const std::string& username,
                                     const std::string& password,
                                     const QString& captchaNonce = {},
                                     int captchaAnswer = -1,
                                     const QString& requestId = {});

        static Message createSuccessResponse(const QJsonObject& userPayload,
                                             const QString& sessionToken,
                                             const QString& requestId = {},
                                             const QString& statusMessage = QStringLiteral("Login successful"));

        static Message createFailureResponse(ErrorCode errorCode,
                                             const QString& reason,
                                             const QString& requestId = {},
                                             const QJsonObject& payload = {});
    };

} // namespace common

#endif // COMMON_PROTOCOL_LOGIN_MESSAGE_H
