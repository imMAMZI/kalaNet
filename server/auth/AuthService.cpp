#include "AuthService.h"
#include "protocol/commands.h"

AuthService::AuthService(UserRepository& repo)
    : repo_(repo)
{
}

static QString roleToString(const QString& role)
{
    return role; // اگر role را رشته ذخیره می‌کنید
}

common::Message AuthService::login(const QJsonObject& payload)
{
    const QString username = payload.value("username").toString();
    const QString password = payload.value("password").toString();

    if (username.isEmpty() || password.isEmpty())
    {
        return common::Message(
            common::Command::LoginResult,
            QJsonObject{
                {"success", false},
                {"message", "Missing username or password"}
            }
        );
    }

    const QString hash = PasswordHasher::hash(password);

    if (!repo_.checkPassword(username, hash))
    {
        return common::Message(
            common::Command::LoginResult,
            QJsonObject{
                {"success", false},
                {"message", "Invalid username or password"}
            }
        );
    }

    User user;
    repo_.getUser(username, user);

    return common::Message(
        common::Command::LoginResult,
        QJsonObject{
            {"success", true},
            {"message", "Login successful"},
            {"fullName", user.fullName},
            {"role", roleToString(user.role)}
        }
    );
}

common::Message AuthService::signup(const QJsonObject& payload)
{
    const QString fullName = payload.value("fullName").toString();
    const QString username = payload.value("username").toString();
    const QString phone    = payload.value("phone").toString();
    const QString email    = payload.value("email").toString();
    const QString password = payload.value("password").toString();

    if (fullName.isEmpty() || username.isEmpty() || phone.isEmpty() ||
        email.isEmpty() || password.isEmpty())
    {
        return common::Message(
            common::Command::SignupResult,
            QJsonObject{
                {"success", false},
                {"message", "Missing required fields"}
            }
        );
    }

    if (password.size() < 8)
    {
        return common::Message(
            common::Command::SignupResult,
            QJsonObject{
                {"success", false},
                {"message", "Password must be at least 8 characters"}
            }
        );
    }

    if (!email.contains("@"))
    {
        return common::Message(
            common::Command::SignupResult,
            QJsonObject{
                {"success", false},
                {"message", "Invalid email format"}
            }
        );
    }

    if (repo_.userExists(username))
    {
        return common::Message(
            common::Command::SignupResult,
            QJsonObject{
                {"success", false},
                {"message", "Username already exists"}
            }
        );
    }

    User user;
    user.fullName     = fullName;
    user.username     = username;
    user.phone        = phone;
    user.email        = email;
    user.passwordHash = PasswordHasher::hash(password);
    user.role         = "User";

    repo_.createUser(user);

    return common::Message(
        common::Command::SignupResult,
        QJsonObject{
            {"success", true},
            {"message", "Signup successful"}
        }
    );
}
