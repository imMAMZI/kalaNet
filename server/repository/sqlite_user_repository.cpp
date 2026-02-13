#include "sqlite_user_repository.h"
#include <QMutexLocker>
#include <QCoreApplication>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

SqliteUserRepository::SqliteUserRepository(const QString& databasePath)
{
    databasePath_ = databasePath;
    if (databasePath_.isEmpty()) {
        databasePath_ = QCoreApplication::applicationDirPath()
                        + QDir::separator()
                        + QStringLiteral("kalanet.db");
    }

    connectionName_ = QStringLiteral("kalanet_repo_%1")
                          .arg(reinterpret_cast<quintptr>(this));

    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                    connectionName_);
    db_.setDatabaseName(databasePath_);

    if (!db_.open()) {
        throwDatabaseError(QStringLiteral("open database"), db_.lastError());
    }

    initializeSchema();
}

SqliteUserRepository::~SqliteUserRepository()
{
    if (db_.isValid()) {
        db_.close();
    }
    QSqlDatabase::removeDatabase(connectionName_);
}

bool SqliteUserRepository::ensureConnection()
{
    if (db_.isOpen())
        return true;

    if (!db_.open()) {
        throwDatabaseError(QStringLiteral("reopen database"), db_.lastError());
    }
    return true;
}

void SqliteUserRepository::initializeSchema()
{
    QMutexLocker locker(&mutex_);

    ensureConnection();

    QSqlQuery pragma(db_);
    if (!pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON;"))) {
        throwDatabaseError(QStringLiteral("enable foreign keys"), pragma.lastError());
    }

    const char* createUsersTable =
        "CREATE TABLE IF NOT EXISTS users ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    full_name    TEXT NOT NULL,"
        "    username     TEXT NOT NULL UNIQUE,"
        "    phone        TEXT NOT NULL,"
        "    email        TEXT NOT NULL UNIQUE,"
        "    passwordHash TEXT NOT NULL,"
        "    role         TEXT NOT NULL"
        ");";

    QSqlQuery create(db_);
    if (!create.exec(QString::fromUtf8(createUsersTable))) {
        throwDatabaseError(QStringLiteral("create users table"), create.lastError());
    }
}

bool SqliteUserRepository::userExists(const QString& username)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT 1 FROM users WHERE username = :username LIMIT 1;"));
    query.bindValue(QStringLiteral(":username"), username);

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("userExists"), query.lastError());
    }

    return query.next();
}

bool SqliteUserRepository::checkPassword(const QString& username,
                                         const QString& passwordHash)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT passwordHash FROM users WHERE username = :username LIMIT 1;"));
    query.bindValue(QStringLiteral(":username"), username);

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("checkPassword"), query.lastError());
    }

    if (!query.next())
        return false;

    const QString storedHash = query.value(0).toString();
    return storedHash == passwordHash;
}

bool SqliteUserRepository::getUser(const QString& username, User& outUser)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT full_name, username, phone, email, passwordHash, role "
        "FROM users WHERE username = :username LIMIT 1;"));
    query.bindValue(QStringLiteral(":username"), username);

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("getUser"), query.lastError());
    }

    if (!query.next())
        return false;

    outUser.fullName     = query.value(0).toString();
    outUser.username     = query.value(1).toString();
    outUser.phone        = query.value(2).toString();
    outUser.email        = query.value(3).toString();
    outUser.passwordHash = query.value(4).toString();
    outUser.role         = query.value(5).toString();

    return true;
}

void SqliteUserRepository::createUser(const User& user)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "INSERT INTO users (full_name, username, phone, email, passwordHash, role) "
        "VALUES (:full_name, :username, :phone, :email, :passwordHash, :role);"));
    query.bindValue(QStringLiteral(":full_name"), user.fullName);
    query.bindValue(QStringLiteral(":username"), user.username);
    query.bindValue(QStringLiteral(":phone"), user.phone);
    query.bindValue(QStringLiteral(":email"), user.email);
    query.bindValue(QStringLiteral(":passwordHash"), user.passwordHash);
    query.bindValue(QStringLiteral(":role"), user.role);

    if (!query.exec()) {
        if (query.lastError().nativeErrorCode() == QStringLiteral("2067")) {
            // SQLITE_CONSTRAINT_UNIQUE
            throw std::runtime_error("Duplicate username or email");
        }
        throwDatabaseError(QStringLiteral("createUser"), query.lastError());
    }
}

[[noreturn]] void SqliteUserRepository::throwDatabaseError(
    const QString& context,
    const QSqlError& error) const
{
    const QString message = QStringLiteral("Database error (%1): %2")
                                .arg(context, error.text());
    throw std::runtime_error(message.toStdString());
}
bool SqliteUserRepository::emailExists(const QString& email)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT 1 FROM users WHERE email = :email LIMIT 1;"));
    query.bindValue(QStringLiteral(":email"), email);

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("emailExists"), query.lastError());
    }

    return query.next();
}
