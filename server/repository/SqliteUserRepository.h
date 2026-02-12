#ifndef SQLITE_USER_REPOSITORY_H
#define SQLITE_USER_REPOSITORY_H

#include "UserRepository.h"
#include <QHash>
#include <QMutex>

class SqliteUserRepository : public UserRepository
{
public:
    SqliteUserRepository();

    bool userExists(const QString& username) override;

    bool checkPassword(
        const QString& username,
        const QString& passwordHash
    ) override;

    bool getUser(
        const QString& username,
        User& outUser
    ) override;

    void createUser(
        const User& user
    ) override;

private:
    QHash<QString, User> users_;
    QMutex mutex_;
};

#endif // SQLITE_USER_REPOSITORY_H
