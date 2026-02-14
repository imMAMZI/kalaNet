#include "cart_service.h"

#include "../repository/cart_repository.h"
#include "../repository/ad_repository.h"

#include <QJsonArray>

#include <exception>

namespace {

QString extractUsername(const QJsonObject& payload)
{
    return payload.value(QStringLiteral("username")).toString().trimmed();
}

int extractAdId(const QJsonObject& payload)
{
    return payload.value(QStringLiteral("adId")).toInt(-1);
}

common::Message validateUserAndAd(const QJsonObject& payload,
                                  common::Command resultCommand,
                                  QString* usernameOut,
                                  int* adIdOut)
{
    const QString username = extractUsername(payload);
    if (username.isEmpty()) {
        return common::Message::makeFailure(
            resultCommand,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("A valid username is required"));
    }

    const int adId = extractAdId(payload);
    if (adId <= 0) {
        return common::Message::makeFailure(
            resultCommand,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("A valid adId is required"));
    }

    *usernameOut = username;
    *adIdOut = adId;
    return common::Message{};
}

} // namespace

CartService::CartService(CartRepository& cartRepository,
                         AdRepository& adRepository)
    : cartRepository_(cartRepository)
    , adRepository_(adRepository)
{
}

common::Message CartService::addItem(const QJsonObject& payload)
{
    QString username;
    int adId = -1;
    const common::Message validation = validateUserAndAd(
        payload,
        common::Command::CartAddItemResult,
        &username,
        &adId);
    if (validation.command() != common::Command::Unknown) {
        return validation;
    }

    try {
        const auto ad = adRepository_.findAdById(adId);
        if (!ad.has_value()) {
            return common::Message::makeFailure(
                common::Command::CartAddItemResult,
                common::ErrorCode::NotFound,
                QStringLiteral("Advertisement not found"));
        }

        const QString status = ad->status.trimmed().toLower();
        if (status != QStringLiteral("approved")) {
            return common::Message::makeFailure(
                common::Command::CartAddItemResult,
                common::ErrorCode::AdNotAvailable,
                QStringLiteral("Only approved and unsold advertisements can be added to cart"));
        }

        const bool inserted = cartRepository_.addItem(username, adId);

        QJsonObject responsePayload;
        responsePayload.insert(QStringLiteral("username"), username);
        responsePayload.insert(QStringLiteral("adId"), adId);
        responsePayload.insert(QStringLiteral("added"), inserted);

        return common::Message::makeSuccess(
            common::Command::CartAddItemResult,
            responsePayload,
            {},
            {},
            inserted
                ? QStringLiteral("Item added to cart")
                : QStringLiteral("Item is already in cart"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(
            common::Command::CartAddItemResult,
            common::ErrorCode::InternalError,
            QStringLiteral("Failed to add item to cart: %1").arg(QString::fromUtf8(ex.what())));
    }
}

common::Message CartService::removeItem(const QJsonObject& payload)
{
    QString username;
    int adId = -1;
    const common::Message validation = validateUserAndAd(
        payload,
        common::Command::CartRemoveItemResult,
        &username,
        &adId);
    if (validation.command() != common::Command::Unknown) {
        return validation;
    }

    try {
        const bool removed = cartRepository_.removeItem(username, adId);

        QJsonObject responsePayload;
        responsePayload.insert(QStringLiteral("username"), username);
        responsePayload.insert(QStringLiteral("adId"), adId);
        responsePayload.insert(QStringLiteral("removed"), removed);

        return common::Message::makeSuccess(
            common::Command::CartRemoveItemResult,
            responsePayload,
            {},
            {},
            removed
                ? QStringLiteral("Item removed from cart")
                : QStringLiteral("Item was not in cart"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(
            common::Command::CartRemoveItemResult,
            common::ErrorCode::InternalError,
            QStringLiteral("Failed to remove item from cart: %1").arg(QString::fromUtf8(ex.what())));
    }
}

common::Message CartService::list(const QJsonObject& payload)
{
    const QString username = extractUsername(payload);
    if (username.isEmpty()) {
        return common::Message::makeFailure(
            common::Command::CartListResult,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("A valid username is required"));
    }

    try {
        const QVector<int> adIds = cartRepository_.listItems(username);
        QJsonArray adIdArray;
        QJsonArray validItemsArray;

        for (const int adId : adIds) {
            const auto ad = adRepository_.findAdById(adId);
            if (!ad.has_value()) {
                continue;
            }

            const QString status = ad->status.trimmed().toLower();
            if (status != QStringLiteral("approved")) {
                continue;
            }

            adIdArray.append(adId);

            QJsonObject adJson;
            adJson.insert(QStringLiteral("adId"), ad->id);
            adJson.insert(QStringLiteral("title"), ad->title);
            adJson.insert(QStringLiteral("category"), ad->category);
            adJson.insert(QStringLiteral("priceTokens"), ad->priceTokens);
            adJson.insert(QStringLiteral("sellerUsername"), ad->sellerUsername);
            adJson.insert(QStringLiteral("status"), ad->status);
            validItemsArray.append(adJson);
        }

        QJsonObject responsePayload;
        responsePayload.insert(QStringLiteral("username"), username);
        responsePayload.insert(QStringLiteral("adIds"), adIdArray);
        responsePayload.insert(QStringLiteral("items"), validItemsArray);
        responsePayload.insert(QStringLiteral("count"), validItemsArray.size());

        return common::Message::makeSuccess(
            common::Command::CartListResult,
            responsePayload,
            {},
            {},
            QStringLiteral("Cart loaded"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(
            common::Command::CartListResult,
            common::ErrorCode::InternalError,
            QStringLiteral("Failed to load cart: %1").arg(QString::fromUtf8(ex.what())));
    }
}

common::Message CartService::clear(const QJsonObject& payload)
{
    const QString username = extractUsername(payload);
    if (username.isEmpty()) {
        return common::Message::makeFailure(
            common::Command::CartClearResult,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("A valid username is required"));
    }

    try {
        const int removedCount = cartRepository_.clearItems(username);

        QJsonObject responsePayload;
        responsePayload.insert(QStringLiteral("username"), username);
        responsePayload.insert(QStringLiteral("clearedCount"), removedCount);

        return common::Message::makeSuccess(
            common::Command::CartClearResult,
            responsePayload,
            {},
            {},
            QStringLiteral("Cart cleared"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(
            common::Command::CartClearResult,
            common::ErrorCode::InternalError,
            QStringLiteral("Failed to clear cart: %1").arg(QString::fromUtf8(ex.what())));
    }
}
