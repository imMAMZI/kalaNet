#ifndef SQLITE_AD_REPOSITORY_H
#define SQLITE_AD_REPOSITORY_H

#include "ad_repository.h"

#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>

class SqliteAdRepository : public AdRepository
{
public:
    explicit SqliteAdRepository(const QString& databasePath = QString());
    ~SqliteAdRepository() override;

    int createPendingAd(const NewAd& ad) override;

private:
    bool ensureConnection();
    void initializeSchema();
    int currentSchemaVersion();
    void applyMigration(int version, const char* statement);
    [[noreturn]] void throwDatabaseError(const QString& context,
                                         const QSqlError& error) const;

    QSqlDatabase db_;
    QString connectionName_;
    QString databasePath_;
    QMutex mutex_;
};

#endif // SQLITE_AD_REPOSITORY_H
