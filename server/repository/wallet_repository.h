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
        QVector<CheckoutItem> purchasedItems;
    };

    virtual ~WalletRepository() = default;

    virtual int getBalance(const QString& username) = 0;
    virtual int topUp(const QString& username, int amountTokens) = 0;
    virtual bool checkout(const QString& buyerUsername,
                          const QVector<int>& adIds,
                          CheckoutResult& result,
                          QString* errorMessage) = 0;
    virtual QVector<LedgerEntry> transactionHistory(const QString& username, int limit) = 0;
};

#endif // WALLET_REPOSITORY_H
