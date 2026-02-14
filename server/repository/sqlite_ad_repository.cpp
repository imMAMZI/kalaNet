#include "sqlite_ad_repository.h"

#include <QCoreApplication>
#include <QDir>
#include <QMutexLocker>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <stdexcept>

namespace {

struct Migration { int version; const char* statement; };

constexpr Migration kMigrations[] = {
    {
        1,
        "CREATE TABLE IF NOT EXISTS ads ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    title TEXT NOT NULL,"
        "    description TEXT NOT NULL,"
        "    category TEXT NOT NULL,"
        "    price_tokens INTEGER NOT NULL CHECK (price_tokens > 0),"
        "    seller_username TEXT NOT NULL,"
        "    image_bytes BLOB,"
        "    status TEXT NOT NULL,"
        "    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");"
    },
    {
        2,
        "CREATE TABLE IF NOT EXISTS ad_status_history ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    ad_id INTEGER NOT NULL,"
        "    previous_status TEXT,"
        "    new_status TEXT NOT NULL,"
        "    changed_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "    reason TEXT,"
        "    FOREIGN KEY(ad_id) REFERENCES ads(id) ON DELETE CASCADE"
        ");"
    },
    {
        3,
        "CREATE INDEX IF NOT EXISTS idx_ads_status_created_at "
        "ON ads(status, created_at DESC);"
    },
    {
        4,
        "CREATE INDEX IF NOT EXISTS idx_ads_seller_username "
        "ON ads(seller_username);"
    },
    {
        5,
        "CREATE INDEX IF NOT EXISTS idx_ad_status_history_ad_id_changed_at "
        "ON ad_status_history(ad_id, changed_at DESC);"
    }
};

constexpr int kLatestSchemaVersion = sizeof(kMigrations) / sizeof(kMigrations[0]);

} // namespace

SqliteAdRepository::SqliteAdRepository(const QString& databasePath)
{
    databasePath_ = databasePath;
    if (databasePath_.isEmpty()) {
        databasePath_ = QCoreApplication::applicationDirPath()
                        + QDir::separator()
                        + QStringLiteral("kalanet.db");
    }

    connectionName_ = QStringLiteral("kalanet_ads_repo_%1")
                          .arg(reinterpret_cast<quintptr>(this));

    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                    connectionName_);
    db_.setDatabaseName(databasePath_);

    if (!db_.open()) {
        throwDatabaseError(QStringLiteral("open database"), db_.lastError());
    }

    initializeSchema();
}

SqliteAdRepository::~SqliteAdRepository()
{
    if (db_.isValid()) {
        db_.close();
    }
    QSqlDatabase::removeDatabase(connectionName_);
}

bool SqliteAdRepository::ensureConnection()
{
    if (db_.isOpen())
        return true;

    if (!db_.open()) {
        throwDatabaseError(QStringLiteral("reopen database"), db_.lastError());
    }
    return true;
}

void SqliteAdRepository::initializeSchema()
{
    QMutexLocker locker(&mutex_);

    ensureConnection();

    QSqlQuery pragma(db_);
    if (!pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON;"))) {
        throwDatabaseError(QStringLiteral("enable foreign keys"), pragma.lastError());
    }

    QSqlQuery migrationTable(db_);
    if (!migrationTable.exec(
            QStringLiteral("CREATE TABLE IF NOT EXISTS schema_migrations ("
                           "    version INTEGER PRIMARY KEY,"
                           "    applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                           ");"))) {
        throwDatabaseError(QStringLiteral("create schema_migrations table"),
                           migrationTable.lastError());
    }

    const int currentVersion = currentSchemaVersion();

    for (const auto& migration : kMigrations) {
        if (migration.version > currentVersion) {
            applyMigration(migration.version, migration.statement);
        }
    }

    if (currentSchemaVersion() != kLatestSchemaVersion) {
        throw std::runtime_error("Ad schema migration did not reach latest version");
    }
}

int SqliteAdRepository::currentSchemaVersion()
{
    QSqlQuery query(db_);
    if (!query.exec(QStringLiteral("SELECT COALESCE(MAX(version), 0) FROM schema_migrations;"))) {
        throwDatabaseError(QStringLiteral("read schema version"), query.lastError());
    }

    if (!query.next()) {
        return 0;
    }

    return query.value(0).toInt();
}

void SqliteAdRepository::applyMigration(int version, const char* statement)
{
    if (!db_.transaction()) {
        throwDatabaseError(QStringLiteral("begin migration transaction"), db_.lastError());
    }

    QSqlQuery query(db_);
    if (!query.exec(QString::fromUtf8(statement))) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("apply migration %1").arg(version),
                           query.lastError());
    }

    QSqlQuery insertMigration(db_);
    insertMigration.prepare(QStringLiteral(
        "INSERT INTO schema_migrations (version) VALUES (:version);"));
    insertMigration.bindValue(QStringLiteral(":version"), version);

    if (!insertMigration.exec()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("record migration %1").arg(version),
                           insertMigration.lastError());
    }

    if (!db_.commit()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("commit migration %1").arg(version),
                           db_.lastError());
    }
}

int SqliteAdRepository::createPendingAd(const NewAd& ad)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    if (!db_.transaction()) {
        throwDatabaseError(QStringLiteral("begin createPendingAd transaction"),
                           db_.lastError());
    }

    QSqlQuery insertAd(db_);
    insertAd.prepare(QStringLiteral(
        "INSERT INTO ads ("
        "    title, description, category, price_tokens,"
        "    seller_username, image_bytes, status"
        ") VALUES ("
        "    :title, :description, :category, :price_tokens,"
        "    :seller_username, :image_bytes, :status"
        ");"));

    insertAd.bindValue(QStringLiteral(":title"), ad.title);
    insertAd.bindValue(QStringLiteral(":description"), ad.description);
    insertAd.bindValue(QStringLiteral(":category"), ad.category);
    insertAd.bindValue(QStringLiteral(":price_tokens"), ad.priceTokens);
    insertAd.bindValue(QStringLiteral(":seller_username"), ad.sellerUsername);
    insertAd.bindValue(QStringLiteral(":image_bytes"), ad.imageBytes);
    insertAd.bindValue(QStringLiteral(":status"), QStringLiteral("pending"));

    if (!insertAd.exec()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("insert ad"), insertAd.lastError());
    }

    const QVariant adIdVariant = insertAd.lastInsertId();
    if (!adIdVariant.isValid()) {
        db_.rollback();
        throw std::runtime_error("Failed to resolve inserted ad ID");
    }

    const int adId = adIdVariant.toInt();

    QSqlQuery insertHistory(db_);
    insertHistory.prepare(QStringLiteral(
        "INSERT INTO ad_status_history (ad_id, previous_status, new_status, reason) "
        "VALUES (:ad_id, NULL, :new_status, :reason);"));
    insertHistory.bindValue(QStringLiteral(":ad_id"), adId);
    insertHistory.bindValue(QStringLiteral(":new_status"), QStringLiteral("pending"));
    insertHistory.bindValue(QStringLiteral(":reason"), QStringLiteral("ad created"));

    if (!insertHistory.exec()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("insert ad status history"),
                           insertHistory.lastError());
    }

    if (!db_.commit()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("commit createPendingAd transaction"),
                           db_.lastError());
    }

    return adId;
}

[[noreturn]] void SqliteAdRepository::throwDatabaseError(
    const QString& context,
    const QSqlError& error) const
{
    const QString message = QStringLiteral("Database error (%1): %2")
                                .arg(context, error.text());
    throw std::runtime_error(message.toStdString());
}
