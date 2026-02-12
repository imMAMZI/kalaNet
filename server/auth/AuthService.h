#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include "protocol/message.h"

class AuthService
{
public:
    AuthService() = default;

    common::Message login(const QJsonObject& payload);
    common::Message signup(const QJsonObject& payload);
};

#endif // AUTH_SERVICE_H
