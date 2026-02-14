#ifndef COMMON_PROTOCOL_AD_CREATE_MESSAGE_H
#define COMMON_PROTOCOL_AD_CREATE_MESSAGE_H

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include "protocol/error_codes.h"
#include "protocol/message.h"

namespace common {

class AdCreateMessage {
public:
    static Message createRequest(const QString& title,
                                 const QString& description,
                                 const QString& category,
                                 int priceTokens,
                                 const QByteArray& imageBytes,
                                 const QString& sellerUsername = {},
                                 const QString& requestId = {});

    static Message createSuccessResponse(int adId,
                                         const QString& requestId = {},
                                         const QString& statusMessage =
                                             QStringLiteral("Ad submitted and marked as pending"));

    static Message createFailureResponse(ErrorCode errorCode,
                                         const QString& reason,
                                         const QString& requestId = {},
                                         const QJsonObject& payload = {});
};

} // namespace common

#endif // COMMON_PROTOCOL_AD_CREATE_MESSAGE_H

