#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include <QString>

struct User
{
    QString fullName;
    QString username;
    QString phone;
    QString email;
    QString passwordHash;
    QString role;
};

class UserRepository
{
public:
    virtual bool userExists(const QString& username) = 0;
    virtual bool emailExists(const QString& email) = 0;

    virtual bool checkPassword(
        const QString& username,
        const QString& passwordHash
    ) = 0;

    virtual bool getUser(
        const QString& username,
        User& outUser
    ) = 0;

    virtual void createUser(
        const User& user
    ) = 0;

    virtual ~UserRepository() = default;
};

#endif // USER_REPOSITORY_H
