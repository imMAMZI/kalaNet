#ifndef SQLITE_WALLET_REPOSITORY_H
#define SQLITE_WALLET_REPOSITORY_H

#include "wallet_repository.h"

#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>

class SqliteWalletRepository : public WalletRepository
{
public:
    explicit SqliteWalletRepository(const QString& databasePath = QString());
    ~SqliteWalletRepository() override;

    int getBalance(const QString& username) override;
    int topUp(const QString& username, int amountTokens) override;
    bool checkout(const QString& buyerUsername,
                  const QVector<int>& adIds,
                  const QString& discountCode,
                  CheckoutResult& result,
                  QString* errorMessage) override;
    DiscountValidationResult validateDiscountCode(const QString& code,
                                                  int subtotalTokens,
                                                  const QString& username = {}) override;
    QVector<DiscountValidationResult> listDiscountCodes() override;
    bool upsertDiscountCode(const DiscountValidationResult& record,
                            QString* errorMessage) override;
    bool deleteDiscountCode(const QString& code,
                            QString* errorMessage) override;
    QVector<LedgerEntry> transactionHistory(const QString& username, int limit) override;

private:
    bool ensureConnection();
    void initializeSchema();
    void ensureWalletRow(const QString& username);
    [[noreturn]] void throwDatabaseError(const QString& context,
                                         const QSqlError& error) const;

    QSqlDatabase db_;
    QString connectionName_;
    QString databasePath_;
    QMutex mutex_;
};

#endif // SQLITE_WALLET_REPOSITORY_H
