#ifndef COMMON_PROTOCOL_ERROR_CODES_H
#define COMMON_PROTOCOL_ERROR_CODES_H

#include <QString>

namespace common {

    enum class ErrorCode {
        None = 0,
        UnknownCommand,
        InvalidJson,
        InvalidPayload,
        AuthInvalidCredentials,
        AuthSessionExpired,
        AuthUnauthorized,
        ValidationFailed,
        NotFound,
        AlreadyExists,
        PermissionDenied,
        InsufficientFunds,
        AdNotAvailable,
        DuplicateAd,
        DatabaseError,
        InternalError
    };

    QString errorCodeToString(ErrorCode code);
    ErrorCode errorCodeFromString(const QString& code);

} // namespace common

#endif // COMMON_PROTOCOL_ERROR_CODES_H
