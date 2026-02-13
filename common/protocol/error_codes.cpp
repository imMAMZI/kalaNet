#include "protocol/error_codes.h"

#include <QHash>

namespace common {

namespace {

const QHash<ErrorCode, QString>& forwardMap()
{
    static const QHash<ErrorCode, QString> map = {
        { ErrorCode::None, QStringLiteral("NONE") },
        { ErrorCode::UnknownCommand, QStringLiteral("UNKNOWN_COMMAND") },
        { ErrorCode::InvalidJson, QStringLiteral("INVALID_JSON") },
        { ErrorCode::InvalidPayload, QStringLiteral("INVALID_PAYLOAD") },
        { ErrorCode::AuthInvalidCredentials, QStringLiteral("AUTH_INVALID_CREDENTIALS") },
        { ErrorCode::AuthSessionExpired, QStringLiteral("AUTH_SESSION_EXPIRED") },
        { ErrorCode::AuthUnauthorized, QStringLiteral("AUTH_UNAUTHORIZED") },
        { ErrorCode::ValidationFailed, QStringLiteral("VALIDATION_FAILED") },
        { ErrorCode::NotFound, QStringLiteral("NOT_FOUND") },
        { ErrorCode::AlreadyExists, QStringLiteral("ALREADY_EXISTS") },
        { ErrorCode::PermissionDenied, QStringLiteral("PERMISSION_DENIED") },
        { ErrorCode::InsufficientFunds, QStringLiteral("INSUFFICIENT_FUNDS") },
        { ErrorCode::AdNotAvailable, QStringLiteral("AD_NOT_AVAILABLE") },
        { ErrorCode::DuplicateAd, QStringLiteral("DUPLICATE_AD") },
        { ErrorCode::DatabaseError, QStringLiteral("DATABASE_ERROR") },
        { ErrorCode::InternalError, QStringLiteral("INTERNAL_ERROR") }
    };

    return map;
}

const QHash<QString, ErrorCode>& backwardMap()
{
    static const QHash<QString, ErrorCode> map = [] {
        QHash<QString, ErrorCode> result;
        const auto& forward = forwardMap();
        for (auto it = forward.cbegin(); it != forward.cend(); ++it) {
            result.insert(it.value(), it.key());
        }
        return result;
    }();

    return map;
}

} // namespace

QString errorCodeToString(ErrorCode code)
{
    return forwardMap().value(code, QStringLiteral("NONE"));
}

ErrorCode errorCodeFromString(const QString& code)
{
    return backwardMap().value(code.toUpper(), ErrorCode::None);
}

} // namespace common
