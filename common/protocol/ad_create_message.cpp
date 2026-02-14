#include "protocol/ad_create_message.h"

#include <QJsonObject>

namespace common {

Message AdCreateMessage::createRequest(const QString& title,
                                       const QString& description,
                                       const QString& category,
                                       int priceTokens,
                                       const QByteArray& imageBytes,
                                       const QString& sellerUsername,
                                       const QString& requestId)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("title"), title);
    payload.insert(QStringLiteral("description"), description);
    payload.insert(QStringLiteral("category"), category);
    payload.insert(QStringLiteral("priceTokens"), priceTokens);
    payload.insert(QStringLiteral("imageBase64"), QString::fromLatin1(imageBytes.toBase64()));

    if (!sellerUsername.isEmpty()) {
        payload.insert(QStringLiteral("sellerUsername"), sellerUsername);
    }

    return Message(Command::AdCreate, payload, requestId);
}

Message AdCreateMessage::createSuccessResponse(int adId,
                                               const QString& requestId,
                                               const QString& statusMessage)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("success"), true);
    payload.insert(QStringLiteral("adId"), adId);
    payload.insert(QStringLiteral("status"), QStringLiteral("PENDING"));
    payload.insert(QStringLiteral("message"), statusMessage);

    return Message::makeSuccess(Command::AdCreateResult, payload, requestId, {}, statusMessage);
}

Message AdCreateMessage::createFailureResponse(ErrorCode errorCode,
                                               const QString& reason,
                                               const QString& requestId,
                                               const QJsonObject& payload)
{
    QJsonObject responsePayload = payload;
    responsePayload.insert(QStringLiteral("success"), false);
    responsePayload.insert(QStringLiteral("message"), reason);

    return Message::makeFailure(Command::AdCreateResult,
                                errorCode,
                                reason,
                                responsePayload,
                                requestId);
}

} // namespace common
