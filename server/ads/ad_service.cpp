#include "ads/ad_service.h"

#include <QMutexLocker>

#include "protocol/ad_create_message.h"

namespace {

bool isInvalidText(const QString& value, int minLength)
{
    return value.trimmed().size() < minLength;
}

} // namespace

common::Message AdService::create(const QJsonObject& payload)
{
    const QString title = payload.value(QStringLiteral("title")).toString().trimmed();
    const QString description = payload.value(QStringLiteral("description")).toString().trimmed();
    const QString category = payload.value(QStringLiteral("category")).toString().trimmed();
    const int priceTokens = payload.value(QStringLiteral("priceTokens")).toInt(0);
    const QString sellerUsername = payload.value(QStringLiteral("sellerUsername")).toString().trimmed();
    const QByteArray imageBytes =
        QByteArray::fromBase64(payload.value(QStringLiteral("imageBase64")).toString().toLatin1());

    if (isInvalidText(title, 3)) {
        return common::AdCreateMessage::createFailureResponse(
            common::ErrorCode::ValidationFailed,
            QStringLiteral("Title must be at least 3 characters"));
    }

    if (isInvalidText(description, 10)) {
        return common::AdCreateMessage::createFailureResponse(
            common::ErrorCode::ValidationFailed,
            QStringLiteral("Description must be at least 10 characters"));
    }

    if (category.isEmpty()) {
        return common::AdCreateMessage::createFailureResponse(
            common::ErrorCode::ValidationFailed,
            QStringLiteral("Category is required"));
    }

    if (priceTokens <= 0) {
        return common::AdCreateMessage::createFailureResponse(
            common::ErrorCode::ValidationFailed,
            QStringLiteral("Price must be greater than zero"));
    }

    QMutexLocker lock(&mutex_);

    StoredAd ad;
    ad.id = nextAdId_++;
    ad.title = title;
    ad.description = description;
    ad.category = category;
    ad.priceTokens = priceTokens;
    ad.sellerUsername = sellerUsername;
    ad.imageBytes = imageBytes;
    pendingAds_.push_back(ad);

    return common::AdCreateMessage::createSuccessResponse(ad.id);
}

