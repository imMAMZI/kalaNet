#ifndef SQLITE_USER_REPOSITORY_H
#define SQLITE_USER_REPOSITORY_H

#include "UserRepository.h"

class SqliteUserRepository : public UserRepository
{
public:
    SqliteUserRepository();

    bool userExists(const QString& username) override;
    bool checkPassword(
        const QString& username,
        const QString& passwordHash
    ) override;
    void createUser(
        const QString& username,
        const QString& passwordHash
    ) override;
};

#endif // SQLITE_USER_REPOSITORY_H
