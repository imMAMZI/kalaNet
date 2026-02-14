#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include "protocol/message.h"
#include "../repository/user_repository.h"
#include "../security/password_hasher.h"
#include <QJsonObject>

class AdRepository;
class WalletRepository;

class AuthService
{
public:
    explicit AuthService(UserRepository& repo,
                         AdRepository* adRepository = nullptr,
                         WalletRepository* walletRepository = nullptr);

    common::Message login(const QJsonObject& payload);
    common::Message signup(const QJsonObject& payload);
    common::Message updateProfile(const QJsonObject& payload);
    common::Message profileHistory(const QJsonObject& payload);
    common::Message adminStats(const QJsonObject& payload);

private:
    UserRepository& repo_;
    AdRepository* adRepository_;
    WalletRepository* walletRepository_;
};

#endif // AUTH_SERVICE_H
