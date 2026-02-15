#ifndef WALLET_REPOSITORY_H
#define WALLET_REPOSITORY_H

#include <QDateTime>
#include <QString>
#include <QVector>

class WalletRepository
{
public:
    struct LedgerEntry {
        int id = -1;
        QString username;
        QString type;
        int amountTokens = 0;
        int balanceAfter = 0;
        int adId = -1;
        QString counterparty;
        QDateTime createdAt;
    };

    struct CheckoutItem {
        int adId = -1;
        QString sellerUsername;
        int priceTokens = 0;
    };

    struct CheckoutResult {
        int buyerBalance = 0;
        int subtotalTokens = 0;
        int discountTokens = 0;
        int totalTokens = 0;
        QString appliedDiscountCode;
        QVector<CheckoutItem> purchasedItems;
    };

    struct DiscountValidationResult {
        bool valid = false;
        QString message;
        QString code;
        QString type;
        int valueTokens = 0;
        int maxDiscountTokens = 0;
        int minSubtotalTokens = 0;
        int subtotalTokens = 0;
        int discountTokens = 0;
        int totalTokens = 0;
        int usageLimit = -1;
        int usedCount = 0;
        bool active = true;
        QDateTime expiresAt;
    };

    virtual ~WalletRepository() = default;

    virtual int getBalance(const QString& username) = 0;
    virtual int topUp(const QString& username, int amountTokens) = 0;
    virtual bool checkout(const QString& buyerUsername,
                          const QVector<int>& adIds,
                          const QString& discountCode,
                          CheckoutResult& result,
                          QString* errorMessage) = 0;
    virtual DiscountValidationResult validateDiscountCode(const QString& code,
                                                          int subtotalTokens,
                                                          const QString& username = {}) = 0;
    virtual QVector<DiscountValidationResult> listDiscountCodes() = 0;
    virtual bool upsertDiscountCode(const DiscountValidationResult& record,
                                    QString* errorMessage) = 0;
    virtual bool deleteDiscountCode(const QString& code,
                                    QString* errorMessage) = 0;
    virtual QVector<LedgerEntry> transactionHistory(const QString& username, int limit) = 0;
};

#endif // WALLET_REPOSITORY_H
