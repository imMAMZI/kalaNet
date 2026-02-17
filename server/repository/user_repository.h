#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include <QString>
#include <QVector>
#include <optional>

struct User
{
    QString fullName;
    QString username;
    QString phone;
    QString email;
    QString passwordHash;
    QString role;
};

struct AdminUserInfo
{
    QString fullName;
    QString username;
    QString phone;
    QString passwordHash;
    int soldItems = 0;
    int boughtItems = 0;
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

    virtual std::optional<User> findByUsername(const QString& username) = 0;

    virtual void createUser(
        const User& user
    ) = 0;

    virtual bool updateUser(const QString& currentUsername, const User& updatedUser) = 0;
    virtual int countAllUsers() = 0;
    virtual int countUsersByRole(const QString& role) = 0;
    virtual QVector<AdminUserInfo> listUsersForAdmin(const QString& searchTerm = {}) = 0;

    virtual ~UserRepository() = default;
};

#endif // USER_REPOSITORY_H
