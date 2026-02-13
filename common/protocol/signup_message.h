#ifndef COMMON_PROTOCOL_SIGNUP_MESSAGE_H
#define COMMON_PROTOCOL_SIGNUP_MESSAGE_H

#include <string>

#include <QJsonObject>
#include <QString>

#include "protocol/error_codes.h"
#include "protocol/message.h"

namespace common {

    class SignupMessage {
    public:
        static Message createRequest(const QString& fullName,
                                 const QString& username,
                                 const QString& phone,
                                 const QString& email,
                                 const QString& password);
        static Message createSuccessResponse(const QJsonObject& userPayload,
                                             const QString& requestId = {},
                                             const QString& statusMessage = QStringLiteral("Signup completed successfully"));

        static Message createFailureResponse(ErrorCode errorCode,
                                             const QString& reason,
                                             const QString& requestId = {},
                                             const QJsonObject& payload = {});
    };

} // namespace common

#endif // COMMON_PROTOCOL_SIGNUP_MESSAGE_H
