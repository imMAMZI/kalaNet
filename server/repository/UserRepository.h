#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include <QString>

class UserRepository
{
public:
    virtual bool userExists(const QString& username) = 0;
    virtual bool checkPassword(
        const QString& username,
        const QString& passwordHash
    ) = 0;
    virtual void createUser(
        const QString& username,
        const QString& passwordHash
    ) = 0;

    virtual ~UserRepository() = default;
};

#endif // USER_REPOSITORY_H
