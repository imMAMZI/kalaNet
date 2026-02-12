#include "SqliteUserRepository.h"

SqliteUserRepository::SqliteUserRepository()
{
}

bool SqliteUserRepository::userExists(const QString& username)
{
    Q_UNUSED(username);
    return false;
}

bool SqliteUserRepository::checkPassword(
    const QString& username,
    const QString& passwordHash
) {
    Q_UNUSED(username);
    Q_UNUSED(passwordHash);
    return true;
}

void SqliteUserRepository::createUser(
    const QString& username,
    const QString& passwordHash
) {
    Q_UNUSED(username);
    Q_UNUSED(passwordHash);
}
