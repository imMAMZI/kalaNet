#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include "protocol/message.h"
#include "../repository/UserRepository.h"
#include "../security/PasswordHasher.h"
#include <QJsonObject>

class AuthService
{
public:
    explicit AuthService(UserRepository& repo);

    common::Message login(const QJsonObject& payload);
    common::Message signup(const QJsonObject& payload);

private:
    UserRepository& repo_;
};

#endif // AUTH_SERVICE_H
