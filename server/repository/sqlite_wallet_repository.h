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
                  CheckoutResult& result,
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
