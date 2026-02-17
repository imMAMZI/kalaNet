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

std::optional<User> SqliteUserRepository::findByUsername(const QString& username)
{
    User user;
    if (!getUser(username, user)) {
        return std::nullopt;
    }
    return user;
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

bool SqliteUserRepository::updateUser(const QString& currentUsername, const User& updatedUser)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "UPDATE users "
        "SET full_name = :full_name, username = :username, phone = :phone, "
        "email = :email, passwordHash = :passwordHash "
        "WHERE username = :current_username;"));
    query.bindValue(QStringLiteral(":full_name"), updatedUser.fullName);
    query.bindValue(QStringLiteral(":username"), updatedUser.username);
    query.bindValue(QStringLiteral(":phone"), updatedUser.phone);
    query.bindValue(QStringLiteral(":email"), updatedUser.email);
    query.bindValue(QStringLiteral(":passwordHash"), updatedUser.passwordHash);
    query.bindValue(QStringLiteral(":current_username"), currentUsername);

    if (!query.exec()) {
        if (query.lastError().nativeErrorCode() == QStringLiteral("2067")) {
            return false;
        }
        throwDatabaseError(QStringLiteral("updateUser"), query.lastError());
    }

    return query.numRowsAffected() > 0;
}

int SqliteUserRepository::countAllUsers()
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    if (!query.exec(QStringLiteral("SELECT COUNT(1) FROM users;"))) {
        throwDatabaseError(QStringLiteral("countAllUsers"), query.lastError());
    }

    return query.next() ? query.value(0).toInt() : 0;
}


QVector<AdminUserInfo> SqliteUserRepository::listUsersForAdmin(const QString& searchTerm)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    const QString normalizedSearch = searchTerm.trimmed();

    QSqlQuery query(db_);
    QString sql = QStringLiteral(
        "SELECT u.full_name, u.username, u.phone, u.passwordHash, u.role, "
        "COALESCE((SELECT COUNT(1) FROM ads a WHERE a.seller_username = u.username AND LOWER(a.status) = 'sold'), 0) AS sold_count, "
        "COALESCE((SELECT COUNT(1) FROM transaction_ledger t WHERE t.username = u.username AND t.type = 'purchase_debit'), 0) AS bought_count "
        "FROM users u");

    if (!normalizedSearch.isEmpty()) {
        sql += QStringLiteral(" WHERE LOWER(u.full_name) LIKE :search OR LOWER(u.username) LIKE :search");
    }

    sql += QStringLiteral(" ORDER BY LOWER(u.full_name), LOWER(u.username);");

    query.prepare(sql);
    if (!normalizedSearch.isEmpty()) {
        query.bindValue(QStringLiteral(":search"), QStringLiteral("%") + normalizedSearch.toLower() + QStringLiteral("%"));
    }

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("listUsersForAdmin"), query.lastError());
    }

    QVector<AdminUserInfo> users;
    while (query.next()) {
        AdminUserInfo info;
        info.fullName = query.value(0).toString();
        info.username = query.value(1).toString();
        info.phone = query.value(2).toString();
        info.passwordHash = query.value(3).toString();
        info.role = query.value(4).toString();
        info.soldItems = query.value(5).toInt();
        info.boughtItems = query.value(6).toInt();
        users.push_back(info);
    }

    return users;
}

int SqliteUserRepository::countUsersByRole(const QString& role)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral("SELECT COUNT(1) FROM users WHERE role = :role;"));
    query.bindValue(QStringLiteral(":role"), role);
    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("countUsersByRole"), query.lastError());
    }

    return query.next() ? query.value(0).toInt() : 0;
}
