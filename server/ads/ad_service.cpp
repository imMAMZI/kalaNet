#include "ad_service.h"

#include "protocol/ad_create_message.h"
#include "../repository/ad_repository.h"

#include <exception>

#include <QJsonArray>

namespace {

bool isInvalidText(const QString& value, int minLength)
{
    return value.trimmed().size() < minLength;
}

} // namespace

AdService::AdService(AdRepository& adRepository)
    : adRepository_(adRepository)
{
}

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

    try {
        AdRepository::NewAd ad;
        ad.title = title;
        ad.description = description;
        ad.category = category;
        ad.priceTokens = priceTokens;
        ad.sellerUsername = sellerUsername;
        ad.imageBytes = imageBytes;

        const int adId = adRepository_.createPendingAd(ad);
        return common::AdCreateMessage::createSuccessResponse(adId);
    } catch (const std::exception& ex) {
        return common::AdCreateMessage::createFailureResponse(
            common::ErrorCode::InternalError,
            QStringLiteral("Failed to store ad: %1").arg(QString::fromUtf8(ex.what())));
    }
}


common::Message AdService::list(const QJsonObject& payload)
{
    const QString name = payload.value(QStringLiteral("name")).toString().trimmed();
    const QString category = payload.value(QStringLiteral("category")).toString().trimmed();
    const int minPriceTokens = payload.value(QStringLiteral("minPriceTokens")).toInt(0);
    const int maxPriceTokens = payload.value(QStringLiteral("maxPriceTokens")).toInt(0);

    if (minPriceTokens < 0 || maxPriceTokens < 0) {
        return common::Message::makeFailure(
            common::Command::AdListResult,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("Price filters must be zero or positive"));
    }

    if (maxPriceTokens > 0 && minPriceTokens > maxPriceTokens) {
        return common::Message::makeFailure(
            common::Command::AdListResult,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("Minimum price cannot be greater than maximum price"));
    }

    try {
        AdRepository::AdListFilters filters;
        filters.nameContains = name;
        filters.category = category;
        filters.minPriceTokens = minPriceTokens;
        filters.maxPriceTokens = maxPriceTokens;

        const QVector<AdRepository::AdSummaryRecord> ads = adRepository_.listApprovedAds(filters);

        QJsonArray adsJson;
        for (const auto& ad : ads) {
            QJsonObject item;
            item.insert(QStringLiteral("id"), ad.id);
            item.insert(QStringLiteral("title"), ad.title);
            item.insert(QStringLiteral("category"), ad.category);
            item.insert(QStringLiteral("priceTokens"), ad.priceTokens);
            item.insert(QStringLiteral("sellerUsername"), ad.sellerUsername);
            item.insert(QStringLiteral("createdAt"), ad.createdAt);
            item.insert(QStringLiteral("hasImage"), ad.hasImage);
            adsJson.push_back(item);
        }

        QJsonObject responsePayload;
        responsePayload.insert(QStringLiteral("ads"), adsJson);
        responsePayload.insert(QStringLiteral("count"), adsJson.size());

        return common::Message::makeSuccess(
            common::Command::AdListResult,
            responsePayload,
            {},
            {},
            QStringLiteral("Advertisements loaded"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(
            common::Command::AdListResult,
            common::ErrorCode::InternalError,
            QStringLiteral("Failed to load advertisements: %1")
                .arg(QString::fromUtf8(ex.what())));
    }
}

common::Message AdService::detail(const QJsonObject& payload)
{
    const int adId = payload.value(QStringLiteral("adId")).toInt(-1);
    if (adId <= 0) {
        return common::Message::makeFailure(
            common::Command::AdDetailResult,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("A valid adId is required"));
    }

    try {
        const std::optional<AdRepository::AdDetailRecord> ad =
            adRepository_.findApprovedAdById(adId);

        if (!ad.has_value()) {
            return common::Message::makeFailure(
                common::Command::AdDetailResult,
                common::ErrorCode::NotFound,
                QStringLiteral("Advertisement not found"));
        }

        QJsonObject responsePayload;
        responsePayload.insert(QStringLiteral("id"), ad->id);
        responsePayload.insert(QStringLiteral("title"), ad->title);
        responsePayload.insert(QStringLiteral("description"), ad->description);
        responsePayload.insert(QStringLiteral("category"), ad->category);
        responsePayload.insert(QStringLiteral("priceTokens"), ad->priceTokens);
        responsePayload.insert(QStringLiteral("sellerUsername"), ad->sellerUsername);
        responsePayload.insert(QStringLiteral("status"), ad->status);
        responsePayload.insert(QStringLiteral("createdAt"), ad->createdAt);
        responsePayload.insert(QStringLiteral("updatedAt"), ad->updatedAt);
        responsePayload.insert(QStringLiteral("imageBase64"), QString::fromLatin1(ad->imageBytes.toBase64()));

        return common::Message::makeSuccess(
            common::Command::AdDetailResult,
            responsePayload,
            {},
            {},
            QStringLiteral("Advertisement detail loaded"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(
            common::Command::AdDetailResult,
            common::ErrorCode::InternalError,
            QStringLiteral("Failed to load advertisement detail: %1")
                .arg(QString::fromUtf8(ex.what())));
    }
}
