#ifndef SQLITE_CART_REPOSITORY_H
#define SQLITE_CART_REPOSITORY_H

#include "cart_repository.h"

#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>

class SqliteCartRepository : public CartRepository
{
public:
    explicit SqliteCartRepository(const QString& databasePath = QString());
    ~SqliteCartRepository() override;

    bool addItem(const QString& username, int adId) override;
    bool removeItem(const QString& username, int adId) override;
    QVector<int> listItems(const QString& username) override;
    int clearItems(const QString& username) override;
    bool hasItem(const QString& username, int adId) override;

private:
    bool ensureConnection();
    void initializeSchema();
    [[noreturn]] void throwDatabaseError(const QString& context,
                                         const QSqlError& error) const;

    QSqlDatabase db_;
    QString connectionName_;
    QString databasePath_;
    QMutex mutex_;
};

#endif // SQLITE_CART_REPOSITORY_H
