#ifndef SQLITE_USER_REPOSITORY_H
#define SQLITE_USER_REPOSITORY_H

#include "user_repository.h"
#include <QMutex>
#include <QSqlDatabase>
#include <optional>

class SqliteUserRepository : public UserRepository
{
public:
    explicit SqliteUserRepository(const QString& databasePath = QString());
    ~SqliteUserRepository() override;

    bool userExists(const QString& username) override;
    bool checkPassword(const QString& username,
                       const QString& passwordHash) override;
    bool emailExists(const QString& email) override;
    bool getUser(const QString& username, User& outUser) override;
    std::optional<User> findByUsername(const QString& username) override;
    void createUser(const User& user) override;
    bool updateUser(const QString& currentUsername, const User& updatedUser) override;
    int countAllUsers() override;
    int countUsersByRole(const QString& role) override;
    QVector<AdminUserInfo> listUsersForAdmin(const QString& searchTerm = {}) override;

private:
    bool ensureConnection();
    void initializeSchema();
    [[noreturn]] void throwDatabaseError(const QString& context,
                                         const QSqlError& error) const;

    QSqlDatabase db_;
    QString connectionName_;
    QString databasePath_;
    QMutex mutex_;
};

#endif // SQLITE_USER_REPOSITORY_H
