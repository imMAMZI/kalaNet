#include "AuthService.h"
#include "protocol/commands.h"

common::Message AuthService::login(const QJsonObject& payload)
{
    Q_UNUSED(payload);

    return common::Message(
        common::Command::LoginResult,
        QJsonObject{
            {"success", true},
            {"message", "Login successful (fake)"}
        }
    );
}

common::Message AuthService::signup(const QJsonObject& payload)
{
    Q_UNUSED(payload);

    return common::Message(
        common::Command::SignupResult,
        QJsonObject{
            {"success", true},
            {"message", "Signup successful (fake)"}
        }
    );
}
