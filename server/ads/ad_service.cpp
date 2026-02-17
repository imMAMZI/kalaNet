#include "ad_service.h"

#include "protocol/ad_create_message.h"
#include "../repository/ad_repository.h"
#include "../logging_audit_logger.h"

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

}

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
    const bool allowAdminView = payload.value(QStringLiteral("allowAdminView")).toBool(false);

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

        QVector<AdRepository::AdSummaryRecord> ads;
        if (allowAdminView) {
            ads = adRepository_.listAdsForModeration(
                filters,
                payload.value(QStringLiteral("status")).toString().trimmed(),
                payload.value(QStringLiteral("onlyWithImage")).toBool(false),
                payload.value(QStringLiteral("seller")).toString().trimmed(),
                payload.value(QStringLiteral("query")).toString().trimmed());
        } else {
            ads = adRepository_.listApprovedAds(filters);
        }

        QJsonArray adsJson;
        for (const auto& ad : ads) {
            QJsonObject item;
            item.insert(QStringLiteral("id"), ad.id);
            item.insert(QStringLiteral("title"), ad.title);
            item.insert(QStringLiteral("category"), ad.category);
            item.insert(QStringLiteral("priceTokens"), ad.priceTokens);
            item.insert(QStringLiteral("sellerUsername"), ad.sellerUsername);
            item.insert(QStringLiteral("status"), ad.status);
            item.insert(QStringLiteral("createdAt"), ad.createdAt);
            item.insert(QStringLiteral("updatedAt"), ad.updatedAt);
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

    const QString moderatorUsername = payload.value(QStringLiteral("moderatorUsername")).toString().trimmed();

    try {
        const std::optional<AdRepository::AdDetailRecord> current = adRepository_.findAdById(adId);
        if (!current.has_value()) {
            AuditLogger::log(QStringLiteral("ads.moderation"), QStringLiteral("failed"),
                             QJsonObject{{QStringLiteral("adId"), adId},
                                         {QStringLiteral("reason"), QStringLiteral("not_found")}});
            return common::Message::makeFailure(
                common::Command::AdStatusUpdate,
                common::ErrorCode::NotFound,
                QStringLiteral("Advertisement not found"));
        }

        if (current->status.compare(QStringLiteral("pending"), Qt::CaseInsensitive) != 0) {
            return common::Message::makeFailure(
                common::Command::AdStatusUpdate,
                common::ErrorCode::ValidationFailed,
                QStringLiteral("Only pending advertisements can be moderated"));
        }

        const QString effectiveReason = reason.isEmpty() && newStatus == AdRepository::AdModerationStatus::Approved
                                            ? QStringLiteral("approved by admin")
                                            : reason;
        const bool updated = adRepository_.updateStatus(adId, newStatus, effectiveReason);
        if (!updated) {
            AuditLogger::log(QStringLiteral("ads.moderation"), QStringLiteral("failed"),
                             QJsonObject{{QStringLiteral("adId"), adId},
                                         {QStringLiteral("reason"), QStringLiteral("update_rejected")}});
            return common::Message::makeFailure(
                common::Command::AdStatusUpdate,
                common::ErrorCode::ValidationFailed,
                QStringLiteral("Advertisement could not be moderated"));
        }

        QJsonObject responsePayload;
        responsePayload.insert(QStringLiteral("adId"), adId);
        responsePayload.insert(QStringLiteral("previousStatus"), current->status.toLower());
        responsePayload.insert(QStringLiteral("status"),
                               newStatus == AdRepository::AdModerationStatus::Approved
                                   ? QStringLiteral("approved")
                                   : QStringLiteral("rejected"));
        responsePayload.insert(QStringLiteral("reason"), effectiveReason);

        AuditLogger::log(QStringLiteral("ads.moderation"), QStringLiteral("success"),
                         QJsonObject{{QStringLiteral("adId"), adId},
                                     {QStringLiteral("moderatorUsername"), moderatorUsername},
                                     {QStringLiteral("status"), responsePayload.value(QStringLiteral("status")).toString()},
                                     {QStringLiteral("reason"), effectiveReason}});
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

    const bool includeUnapproved = payload.value(QStringLiteral("includeUnapproved")).toBool(false);
    const bool includeHistory = payload.value(QStringLiteral("includeHistory")).toBool(includeUnapproved);

    try {
        const std::optional<AdRepository::AdDetailRecord> ad = includeUnapproved
            ? adRepository_.findAdById(adId)
            : adRepository_.findApprovedAdById(adId);

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

        if (includeHistory) {
            QJsonArray history;
            const QVector<AdRepository::AdStatusHistoryRecord> items = adRepository_.getStatusHistory(adId);
            for (const auto& item : items) {
                history.append(QJsonObject{{QStringLiteral("previousStatus"), item.previousStatus},
                                           {QStringLiteral("newStatus"), item.newStatus},
                                           {QStringLiteral("reason"), item.reason},
                                           {QStringLiteral("changedAt"), item.changedAt}});
            }
            responsePayload.insert(QStringLiteral("statusHistory"), history);
        }

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
