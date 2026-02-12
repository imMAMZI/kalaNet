#include "SqliteUserRepository.h"
#include <QMutexLocker>

SqliteUserRepository::SqliteUserRepository()
{
}

bool SqliteUserRepository::userExists(const QString& username)
{
    QMutexLocker locker(&mutex_);
    return users_.contains(username);
}

bool SqliteUserRepository::checkPassword(
    const QString& username,
    const QString& passwordHash
) {
    QMutexLocker locker(&mutex_);
    if (!users_.contains(username))
        return false;

    return users_[username].passwordHash == passwordHash;
}

bool SqliteUserRepository::getUser(
    const QString& username,
    User& outUser
) {
    QMutexLocker locker(&mutex_);
    if (!users_.contains(username))
        return false;

    outUser = users_[username];
    return true;
}

void SqliteUserRepository::createUser(
    const User& user
) {
    QMutexLocker locker(&mutex_);
    users_.insert(user.username, user);
}
