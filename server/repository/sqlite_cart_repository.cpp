#include "sqlite_cart_repository.h"

#include <QCoreApplication>
#include <QDir>
#include <QMutexLocker>
#include <QSqlQuery>
#include <QVariant>

#include <stdexcept>

SqliteCartRepository::SqliteCartRepository(const QString& databasePath)
{
    databasePath_ = databasePath;
    if (databasePath_.isEmpty()) {
        databasePath_ = QCoreApplication::applicationDirPath()
                        + QDir::separator()
                        + QStringLiteral("kalanet.db");
    }

    connectionName_ = QStringLiteral("kalanet_cart_repo_%1")
                          .arg(reinterpret_cast<quintptr>(this));

    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    db_.setDatabaseName(databasePath_);

    if (!db_.open()) {
        throwDatabaseError(QStringLiteral("open database"), db_.lastError());
    }

    initializeSchema();
}

SqliteCartRepository::~SqliteCartRepository()
{
    if (db_.isValid()) {
        db_.close();
    }
    QSqlDatabase::removeDatabase(connectionName_);
}

bool SqliteCartRepository::ensureConnection()
{
    if (db_.isOpen()) {
        return true;
    }

    if (!db_.open()) {
        throwDatabaseError(QStringLiteral("reopen database"), db_.lastError());
    }
    return true;
}

void SqliteCartRepository::initializeSchema()
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery create(db_);
    if (!create.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS cart_items ("
            "  username TEXT NOT NULL,"
            "  ad_id INTEGER NOT NULL,"
            "  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "  PRIMARY KEY(username, ad_id)"
            ");"))) {
        throwDatabaseError(QStringLiteral("create cart_items table"), create.lastError());
    }

    QSqlQuery createIndex(db_);
    if (!createIndex.exec(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_cart_items_username_created "
            "ON cart_items(username, created_at DESC);"))) {
        throwDatabaseError(QStringLiteral("create cart_items index"), createIndex.lastError());
    }
}

bool SqliteCartRepository::addItem(const QString& username, int adId)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO cart_items (username, ad_id) VALUES (:username, :ad_id);"));
    query.bindValue(QStringLiteral(":username"), username.trimmed());
    query.bindValue(QStringLiteral(":ad_id"), adId);

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("add cart item"), query.lastError());
    }

    return query.numRowsAffected() > 0;
}

bool SqliteCartRepository::removeItem(const QString& username, int adId)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "DELETE FROM cart_items WHERE username = :username AND ad_id = :ad_id;"));
    query.bindValue(QStringLiteral(":username"), username.trimmed());
    query.bindValue(QStringLiteral(":ad_id"), adId);

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("remove cart item"), query.lastError());
    }

    return query.numRowsAffected() > 0;
}

QVector<int> SqliteCartRepository::listItems(const QString& username)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT ad_id FROM cart_items WHERE username = :username ORDER BY created_at DESC, ad_id DESC;"));
    query.bindValue(QStringLiteral(":username"), username.trimmed());

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("list cart items"), query.lastError());
    }

    QVector<int> adIds;
    while (query.next()) {
        adIds.push_back(query.value(0).toInt());
    }
    return adIds;
}

int SqliteCartRepository::clearItems(const QString& username)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral("DELETE FROM cart_items WHERE username = :username;"));
    query.bindValue(QStringLiteral(":username"), username.trimmed());

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("clear cart items"), query.lastError());
    }

    return query.numRowsAffected();
}

bool SqliteCartRepository::hasItem(const QString& username, int adId)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT 1 FROM cart_items WHERE username = :username AND ad_id = :ad_id LIMIT 1;"));
    query.bindValue(QStringLiteral(":username"), username.trimmed());
    query.bindValue(QStringLiteral(":ad_id"), adId);

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("check cart item"), query.lastError());
    }

    return query.next();
}

[[noreturn]] void SqliteCartRepository::throwDatabaseError(
    const QString& context,
    const QSqlError& error) const
{
    const QString message = QStringLiteral("Database error (%1): %2")
                                .arg(context, error.text());
    throw std::runtime_error(message.toStdString());
}
