#include "wallet_service.h"

#include "../repository/wallet_repository.h"
#include "../logging_audit_logger.h"

#include <QJsonArray>

#include <exception>
#include <limits>

namespace {

QString usernameFromPayload(const QJsonObject& payload)
{
    return payload.value(QStringLiteral("username")).toString().trimmed();
}

common::ErrorCode mapCheckoutFailureCode(const QString& error)
{
    const QString normalized = error.toLower();
    if (normalized.contains(QStringLiteral("insufficient"))) {
        return common::ErrorCode::InsufficientFunds;
    }
    if (normalized.contains(QStringLiteral("not found"))) {
        return common::ErrorCode::NotFound;
    }
    if (normalized.contains(QStringLiteral("own advertisement"))) {
        return common::ErrorCode::PermissionDenied;
    }
    if (normalized.contains(QStringLiteral("not available")) || normalized.contains(QStringLiteral("no longer available"))) {
        return common::ErrorCode::AdNotAvailable;
    }
    return common::ErrorCode::InternalError;
}

QVector<int> adIdsFromPayload(const QJsonObject& payload)
{
    const QJsonArray adArray = payload.value(QStringLiteral("adIds")).toArray().isEmpty()
                                   ? payload.value(QStringLiteral("cart")).toArray()
                                   : payload.value(QStringLiteral("adIds")).toArray();
    QVector<int> adIds;
    adIds.reserve(adArray.size());
    for (const QJsonValue& value : adArray) {
        const int id = value.toInt(-1);
        if (id > 0 && !adIds.contains(id)) {
            adIds.push_back(id);
        }
    }
    return adIds;
}

} // namespace

WalletService::WalletService(WalletRepository& walletRepository)
    : walletRepository_(walletRepository)
{
}

common::Message WalletService::walletBalance(const QJsonObject& payload)
{
    const QString username = usernameFromPayload(payload);
    if (username.isEmpty()) {
        return common::Message::makeFailure(common::Command::WalletBalanceResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("A valid username is required"));
    }

    try {
        const int balance = walletRepository_.getBalance(username);
        QJsonObject responsePayload;
        responsePayload.insert(QStringLiteral("username"), username);
        responsePayload.insert(QStringLiteral("balanceTokens"), balance);
        return common::Message::makeSuccess(common::Command::WalletBalanceResult,
                                            responsePayload,
                                            {},
                                            {},
                                            QStringLiteral("Wallet balance loaded"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(common::Command::WalletBalanceResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("Failed to load wallet balance: %1").arg(ex.what()));
    }
}

common::Message WalletService::walletTopUp(const QJsonObject& payload)
{
    const QString username = usernameFromPayload(payload);
    const int amountTokens = payload.value(QStringLiteral("amountTokens")).toInt(0);
    const int left = payload.value(QStringLiteral("captchaLeft")).toInt();
    const int right = payload.value(QStringLiteral("captchaRight")).toInt();
    const int answer = payload.value(QStringLiteral("captchaAnswer")).toInt(std::numeric_limits<int>::min());

    if (username.isEmpty() || amountTokens <= 0) {
        return common::Message::makeFailure(common::Command::WalletTopUpResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("Valid username and amountTokens are required"));
    }

    if (answer != (left + right)) {
        return common::Message::makeFailure(common::Command::WalletTopUpResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("CAPTCHA verification failed"));
    }

    try {
        const int newBalance = walletRepository_.topUp(username, amountTokens);
        QJsonObject responsePayload;
        responsePayload.insert(QStringLiteral("username"), username);
        responsePayload.insert(QStringLiteral("creditedTokens"), amountTokens);
        responsePayload.insert(QStringLiteral("balanceTokens"), newBalance);
        return common::Message::makeSuccess(common::Command::WalletTopUpResult,
                                            responsePayload,
                                            {},
                                            {},
                                            QStringLiteral("Wallet top-up completed"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(common::Command::WalletTopUpResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("Wallet top-up failed: %1").arg(ex.what()));
    }
}

common::Message WalletService::buy(const QJsonObject& payload,
                                   QSet<QString>* affectedUsernames,
                                   QVector<int>* soldAdIds)
{
    const QString username = usernameFromPayload(payload);
    const QVector<int> adIds = adIdsFromPayload(payload);

    if (username.isEmpty() || adIds.isEmpty()) {
        return common::Message::makeFailure(common::Command::BuyResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("Valid username and at least one adId are required"));
    }

    try {
        WalletRepository::CheckoutResult checkoutResult;
        QString checkoutError;
        const bool success = walletRepository_.checkout(username, adIds, checkoutResult, &checkoutError);

        if (!success) {
            const QString message = checkoutError.isEmpty() ? QStringLiteral("Checkout failed") : checkoutError;
            const common::ErrorCode code = mapCheckoutFailureCode(message);
            AuditLogger::log(QStringLiteral("purchase.checkout"), QStringLiteral("failed"),
                             QJsonObject{{QStringLiteral("buyer"), username},
                                         {QStringLiteral("adCount"), adIds.size()},
                                         {QStringLiteral("error"), message},
                                         {QStringLiteral("errorCode"), common::errorCodeToString(code)}});
            return common::Message::makeFailure(common::Command::BuyResult,
                                                code,
                                                message);
        }

        if (affectedUsernames) {
            affectedUsernames->insert(username);
            for (const auto& item : checkoutResult.purchasedItems) {
                affectedUsernames->insert(item.sellerUsername);
            }
        }

        QJsonArray purchasedArray;
        QJsonArray soldIds;
        for (const auto& item : checkoutResult.purchasedItems) {
            QJsonObject itemJson;
            itemJson.insert(QStringLiteral("adId"), item.adId);
            itemJson.insert(QStringLiteral("sellerUsername"), item.sellerUsername);
            itemJson.insert(QStringLiteral("priceTokens"), item.priceTokens);
            purchasedArray.append(itemJson);
            soldIds.append(item.adId);
            if (soldAdIds) {
                soldAdIds->push_back(item.adId);
            }
        }

        QJsonObject responsePayload;
        responsePayload.insert(QStringLiteral("username"), username);
        responsePayload.insert(QStringLiteral("balanceTokens"), checkoutResult.buyerBalance);
        responsePayload.insert(QStringLiteral("purchasedItems"), purchasedArray);
        responsePayload.insert(QStringLiteral("soldAdIds"), soldIds);

        AuditLogger::log(QStringLiteral("purchase.checkout"), QStringLiteral("success"),
                         QJsonObject{{QStringLiteral("buyer"), username},
                                     {QStringLiteral("itemCount"), checkoutResult.purchasedItems.size()},
                                     {QStringLiteral("remainingBalance"), checkoutResult.buyerBalance}});
        return common::Message::makeSuccess(common::Command::BuyResult,
                                            responsePayload,
                                            {},
                                            {},
                                            QStringLiteral("Checkout completed successfully"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(common::Command::BuyResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("Checkout failed: %1").arg(ex.what()));
    }
}

common::Message WalletService::transactionHistory(const QJsonObject& payload)
{
    const QString username = usernameFromPayload(payload);
    const int limit = payload.value(QStringLiteral("limit")).toInt(50);

    if (username.isEmpty()) {
        return common::Message::makeFailure(common::Command::TransactionHistoryResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("A valid username is required"));
    }

    try {
        const auto history = walletRepository_.transactionHistory(username, limit);
        QJsonArray items;
        for (const auto& entry : history) {
            QJsonObject item;
            item.insert(QStringLiteral("id"), entry.id);
            item.insert(QStringLiteral("username"), entry.username);
            item.insert(QStringLiteral("type"), entry.type);
            item.insert(QStringLiteral("amountTokens"), entry.amountTokens);
            item.insert(QStringLiteral("balanceAfter"), entry.balanceAfter);
            item.insert(QStringLiteral("adId"), entry.adId > 0 ? entry.adId : QJsonValue());
            item.insert(QStringLiteral("counterparty"), entry.counterparty);
            item.insert(QStringLiteral("createdAt"), entry.createdAt.isValid()
                                                     ? entry.createdAt.toString(Qt::ISODate)
                                                     : QString());
            items.append(item);
        }

        QJsonObject responsePayload;
        responsePayload.insert(QStringLiteral("username"), username);
        responsePayload.insert(QStringLiteral("items"), items);
        responsePayload.insert(QStringLiteral("count"), items.size());

        return common::Message::makeSuccess(common::Command::TransactionHistoryResult,
                                            responsePayload,
                                            {},
                                            {},
                                            QStringLiteral("Transaction history loaded"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(common::Command::TransactionHistoryResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("Failed to load transaction history: %1").arg(ex.what()));
    }
}
