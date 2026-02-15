#include "wallet_service.h"

#include "../repository/wallet_repository.h"
#include "../security/captcha_service.h"
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
    if (normalized.contains(QStringLiteral("discount"))) {
        return common::ErrorCode::ValidationFailed;
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
    for (const auto& value : adArray) {
        if (!value.isDouble()) {
            continue;
        }
        const int adId = value.toInt(-1);
        if (adId > 0) {
            adIds.push_back(adId);
        }
    }
    return adIds;
}

}

WalletService::WalletService(WalletRepository& walletRepository,
                             CaptchaService& captchaService)
    : walletRepository_(walletRepository),
      captchaService_(captchaService)
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
    const int amount = payload.value(QStringLiteral("amountTokens")).toInt(0);

    if (username.isEmpty() || amount <= 0) {
        return common::Message::makeFailure(common::Command::WalletTopUpResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("A valid username and positive amountTokens are required"));
    }

    const QString captchaNonce = payload.value(QStringLiteral("captchaNonce")).toString().trimmed();
    const QString captchaResponse = payload.value(QStringLiteral("captchaResponse")).toString().trimmed();

    if (amount > 500) {
        if (captchaNonce.isEmpty() || captchaResponse.isEmpty()) {
            return common::Message::makeFailure(common::Command::WalletTopUpResult,
                                                common::ErrorCode::CaptchaRequired,
                                                QStringLiteral("CAPTCHA is required for top-up amounts greater than 500 tokens"));
        }

        const auto verifyResult = captchaService_.verify(QStringLiteral("wallet_topup"), captchaNonce, captchaResponse);
        if (!verifyResult.ok) {
            return common::Message::makeFailure(common::Command::WalletTopUpResult,
                                                common::ErrorCode::CaptchaInvalid,
                                                verifyResult.reason.isEmpty()
                                                    ? QStringLiteral("CAPTCHA verification failed")
                                                    : verifyResult.reason);
        }
    }

    if (amount > std::numeric_limits<int>::max() / 2) {
        return common::Message::makeFailure(common::Command::WalletTopUpResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("amountTokens is too large"));
    }

    try {
        const int balance = walletRepository_.topUp(username, amount);
        QJsonObject responsePayload;
        responsePayload.insert(QStringLiteral("username"), username);
        responsePayload.insert(QStringLiteral("balanceTokens"), balance);
        responsePayload.insert(QStringLiteral("amountTokens"), amount);

        AuditLogger::log(QStringLiteral("wallet.topup"), QStringLiteral("success"),
                         QJsonObject{{QStringLiteral("username"), username},
                                     {QStringLiteral("amountTokens"), amount},
                                     {QStringLiteral("balanceTokens"), balance}});

        return common::Message::makeSuccess(common::Command::WalletTopUpResult,
                                            responsePayload,
                                            {},
                                            {},
                                            QStringLiteral("Wallet top-up successful"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(common::Command::WalletTopUpResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("Top-up failed: %1").arg(ex.what()));
    }
}

common::Message WalletService::validateDiscountCode(const QJsonObject& payload)
{
    const QString code = payload.value(QStringLiteral("code")).toString().trimmed();
    const int subtotal = payload.value(QStringLiteral("subtotalTokens")).toInt(0);
    const QString username = usernameFromPayload(payload);

    try {
        const auto result = walletRepository_.validateDiscountCode(code, subtotal, username);
        QJsonObject out;
        out.insert(QStringLiteral("code"), result.code);
        out.insert(QStringLiteral("valid"), result.valid);
        out.insert(QStringLiteral("message"), result.message);
        out.insert(QStringLiteral("type"), result.type);
        out.insert(QStringLiteral("valueTokens"), result.valueTokens);
        out.insert(QStringLiteral("maxDiscountTokens"), result.maxDiscountTokens);
        out.insert(QStringLiteral("minSubtotalTokens"), result.minSubtotalTokens);
        out.insert(QStringLiteral("subtotalTokens"), result.subtotalTokens);
        out.insert(QStringLiteral("discountTokens"), result.discountTokens);
        out.insert(QStringLiteral("totalTokens"), result.totalTokens);
        out.insert(QStringLiteral("usageLimit"), result.usageLimit);
        out.insert(QStringLiteral("usedCount"), result.usedCount);
        out.insert(QStringLiteral("active"), result.active);
        if (result.expiresAt.isValid()) {
            out.insert(QStringLiteral("expiresAt"), result.expiresAt.toUTC().toString(Qt::ISODate));
        }

        return common::Message::makeSuccess(common::Command::DiscountCodeValidateResult,
                                            out,
                                            {},
                                            {},
                                            result.message);
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(common::Command::DiscountCodeValidateResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("Discount validation failed: %1").arg(ex.what()));
    }
}

common::Message WalletService::listDiscountCodes()
{
    try {
        const auto rows = walletRepository_.listDiscountCodes();
        QJsonArray items;
        for (const auto& row : rows) {
            QJsonObject item;
            item.insert(QStringLiteral("code"), row.code);
            item.insert(QStringLiteral("type"), row.type);
            item.insert(QStringLiteral("valueTokens"), row.valueTokens);
            item.insert(QStringLiteral("maxDiscountTokens"), row.maxDiscountTokens);
            item.insert(QStringLiteral("minSubtotalTokens"), row.minSubtotalTokens);
            item.insert(QStringLiteral("usageLimit"), row.usageLimit);
            item.insert(QStringLiteral("usedCount"), row.usedCount);
            item.insert(QStringLiteral("active"), row.active);
            if (row.expiresAt.isValid()) {
                item.insert(QStringLiteral("expiresAt"), row.expiresAt.toUTC().toString(Qt::ISODate));
            }
            items.append(item);
        }

        return common::Message::makeSuccess(common::Command::DiscountCodeListResult,
                                            QJsonObject{{QStringLiteral("items"), items},
                                                        {QStringLiteral("count"), items.size()}},
                                            {},
                                            {},
                                            QStringLiteral("Discount codes loaded"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(common::Command::DiscountCodeListResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("Failed to load discount codes: %1").arg(ex.what()));
    }
}

common::Message WalletService::upsertDiscountCode(const QJsonObject& payload)
{
    WalletRepository::DiscountValidationResult record;
    record.code = payload.value(QStringLiteral("code")).toString().trimmed().toUpper();
    record.type = payload.value(QStringLiteral("type")).toString().trimmed().toLower();
    record.valueTokens = payload.value(QStringLiteral("valueTokens")).toInt(0);
    record.maxDiscountTokens = payload.value(QStringLiteral("maxDiscountTokens")).toInt(0);
    record.minSubtotalTokens = payload.value(QStringLiteral("minSubtotalTokens")).toInt(0);
    record.usageLimit = payload.contains(QStringLiteral("usageLimit"))
                            ? payload.value(QStringLiteral("usageLimit")).toInt(-1)
                            : -1;
    record.active = payload.value(QStringLiteral("active")).toBool(true);
    record.usedCount = qMax(0, payload.value(QStringLiteral("usedCount")).toInt(0));
    record.expiresAt = QDateTime::fromString(payload.value(QStringLiteral("expiresAt")).toString(), Qt::ISODate);

    QString error;
    if (!walletRepository_.upsertDiscountCode(record, &error)) {
        return common::Message::makeFailure(common::Command::DiscountCodeUpsertResult,
                                            common::ErrorCode::ValidationFailed,
                                            error.isEmpty() ? QStringLiteral("Failed to save discount code") : error);
    }

    AuditLogger::log(QStringLiteral("discount.upsert"), QStringLiteral("success"),
                     QJsonObject{{QStringLiteral("code"), record.code},
                                 {QStringLiteral("type"), record.type},
                                 {QStringLiteral("valueTokens"), record.valueTokens}});

    return common::Message::makeSuccess(common::Command::DiscountCodeUpsertResult,
                                        QJsonObject{{QStringLiteral("code"), record.code}},
                                        {},
                                        {},
                                        QStringLiteral("Discount code saved"));
}

common::Message WalletService::deleteDiscountCode(const QJsonObject& payload)
{
    const QString code = payload.value(QStringLiteral("code")).toString().trimmed();
    if (code.isEmpty()) {
        return common::Message::makeFailure(common::Command::DiscountCodeDeleteResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("Discount code is required"));
    }

    QString error;
    if (!walletRepository_.deleteDiscountCode(code, &error)) {
        return common::Message::makeFailure(common::Command::DiscountCodeDeleteResult,
                                            common::ErrorCode::NotFound,
                                            error.isEmpty() ? QStringLiteral("Failed to delete discount code") : error);
    }

    AuditLogger::log(QStringLiteral("discount.delete"), QStringLiteral("success"),
                     QJsonObject{{QStringLiteral("code"), code.toUpper()}});

    return common::Message::makeSuccess(common::Command::DiscountCodeDeleteResult,
                                        QJsonObject{{QStringLiteral("code"), code.toUpper()}},
                                        {},
                                        {},
                                        QStringLiteral("Discount code deleted"));
}

common::Message WalletService::buy(const QJsonObject& payload,
                                   QSet<QString>* affectedUsernames,
                                   QVector<int>* soldAdIds)
{
    const QString username = usernameFromPayload(payload);
    const QVector<int> adIds = adIdsFromPayload(payload);
    const QString discountCode = payload.value(QStringLiteral("discountCode")).toString().trimmed().toUpper();

    if (username.isEmpty() || adIds.isEmpty()) {
        return common::Message::makeFailure(common::Command::BuyResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("Valid username and at least one adId are required"));
    }

    try {
        WalletRepository::CheckoutResult checkoutResult;
        QString checkoutError;
        const bool success = walletRepository_.checkout(username, adIds, discountCode, checkoutResult, &checkoutError);

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
        responsePayload.insert(QStringLiteral("subtotalTokens"), checkoutResult.subtotalTokens);
        responsePayload.insert(QStringLiteral("discountTokens"), checkoutResult.discountTokens);
        responsePayload.insert(QStringLiteral("totalTokens"), checkoutResult.totalTokens);
        responsePayload.insert(QStringLiteral("discountCode"), checkoutResult.appliedDiscountCode);

        AuditLogger::log(QStringLiteral("purchase.checkout"), QStringLiteral("success"),
                         QJsonObject{{QStringLiteral("buyer"), username},
                                     {QStringLiteral("itemCount"), checkoutResult.purchasedItems.size()},
                                     {QStringLiteral("subtotalTokens"), checkoutResult.subtotalTokens},
                                     {QStringLiteral("discountTokens"), checkoutResult.discountTokens},
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
