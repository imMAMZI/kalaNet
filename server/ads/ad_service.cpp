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


AdRepository::AdListSortField parseSortField(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("title") || normalized == QStringLiteral("name")) {
        return AdRepository::AdListSortField::Title;
    }
    if (normalized == QStringLiteral("price") || normalized == QStringLiteral("pricetokens")) {
        return AdRepository::AdListSortField::PriceTokens;
    }
    return AdRepository::AdListSortField::CreatedAt;
}

AdRepository::SortOrder parseSortOrder(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("asc") || normalized == QStringLiteral("ascending")) {
        return AdRepository::SortOrder::Asc;
    }
    return AdRepository::SortOrder::Desc;
}

AdRepository::AdModerationStatus parseModerationStatus(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("approve") || normalized == QStringLiteral("approved")) {
        return AdRepository::AdModerationStatus::Approved;
    }
    if (normalized == QStringLiteral("reject") || normalized == QStringLiteral("rejected")) {
        return AdRepository::AdModerationStatus::Rejected;
    }
    if (normalized == QStringLiteral("sold")) {
        return AdRepository::AdModerationStatus::Sold;
    }
    return AdRepository::AdModerationStatus::Unknown;
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

        if (adRepository_.hasDuplicateActiveAdForSeller(ad)) {
            return common::AdCreateMessage::createFailureResponse(
                common::ErrorCode::DuplicateAd,
                QStringLiteral("Duplicate ad detected for this seller"));
        }

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
        filters.sortField = parseSortField(payload.value(QStringLiteral("sortBy")).toString());
        filters.sortOrder = parseSortOrder(payload.value(QStringLiteral("sortOrder")).toString());

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



common::Message AdService::updateStatus(const QJsonObject& payload)
{
    const int adId = payload.value(QStringLiteral("adId")).toInt(-1);
    if (adId <= 0) {
        return common::Message::makeFailure(
            common::Command::AdStatusUpdate,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("A valid adId is required"));
    }

    const QString action = payload.value(QStringLiteral("action")).toString();
    const AdRepository::AdModerationStatus newStatus = parseModerationStatus(action);
    if (newStatus == AdRepository::AdModerationStatus::Unknown) {
        return common::Message::makeFailure(
            common::Command::AdStatusUpdate,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("Action must be either approve or reject"));
    }

    if (newStatus == AdRepository::AdModerationStatus::Sold) {
        return common::Message::makeFailure(
            common::Command::AdStatusUpdate,
            common::ErrorCode::PermissionDenied,
            QStringLiteral("Sold status can only be set by a successful purchase"));
    }

    const QString reason = payload.value(QStringLiteral("reason")).toString().trimmed();
    if (newStatus == AdRepository::AdModerationStatus::Rejected && reason.isEmpty()) {
        return common::Message::makeFailure(
            common::Command::AdStatusUpdate,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("Rejection reason is required"));
    }

    try {
        const bool updated = adRepository_.updateStatus(adId, newStatus, reason);
        if (!updated) {
            return common::Message::makeFailure(
                common::Command::AdStatusUpdate,
                common::ErrorCode::NotFound,
                QStringLiteral("Advertisement not found"));
        }

        QJsonObject responsePayload;
        responsePayload.insert(QStringLiteral("adId"), adId);
        responsePayload.insert(QStringLiteral("status"),
                               newStatus == AdRepository::AdModerationStatus::Approved
                                   ? QStringLiteral("approved")
                                   : QStringLiteral("rejected"));

        return common::Message::makeSuccess(
            common::Command::AdStatusUpdate,
            responsePayload,
            {},
            {},
            QStringLiteral("Advertisement moderation updated"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(
            common::Command::AdStatusUpdate,
            common::ErrorCode::InternalError,
            QStringLiteral("Failed to update advertisement status: %1")
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
