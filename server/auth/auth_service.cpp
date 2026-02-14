#include "auth_service.h"
#include "protocol/commands.h"
#include <stdexcept>

AuthService::AuthService(UserRepository& repo)
    : repo_(repo)
{
}

static QString roleToString(const QString& role)
{
    return role;
}

common::Message AuthService::login(const QJsonObject& payload)
{
    const QString username = payload.value("username").toString();
    const QString password = payload.value("password").toString();

    if (username.isEmpty() || password.isEmpty())
    {
        return common::Message::makeFailure(
            common::Command::LoginResult,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("Missing username or password"),
            QJsonObject{{"success", false}}
        );
    }

    try
    {
        User user;
        if (!repo_.getUser(username, user))
        {
            return common::Message::makeFailure(
                common::Command::LoginResult,
                common::ErrorCode::NotFound,
                QStringLiteral("User not found"),
                QJsonObject{{"success", false}}
            );
        }

        if (!PasswordHasher::verify(password, user.passwordHash))
        {
            return common::Message::makeFailure(
                common::Command::LoginResult,
                common::ErrorCode::AuthInvalidCredentials,
                QStringLiteral("Invalid username or password"),
                QJsonObject{{"success", false}}
            );
        }

        return common::Message::makeSuccess(
            common::Command::LoginResult,
            QJsonObject{
                {"success", true},
                {"fullName", user.fullName},
                {"role", roleToString(user.role)}
            },
            {},
            {},
            QStringLiteral("Login successful")
        );
    }
    catch (const std::exception&)
    {
        return common::Message::makeFailure(
            common::Command::LoginResult,
            common::ErrorCode::InternalError,
            QStringLiteral("Login failed due to server error"),
            QJsonObject{{"success", false}}
        );
    }
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
        return common::Message::makeFailure(
            common::Command::SignupResult,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("Missing required fields"),
            QJsonObject{{"success", false}}
        );
    }

    if (password.size() < 8)
    {
        return common::Message::makeFailure(
            common::Command::SignupResult,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("Password must be at least 8 characters"),
            QJsonObject{{"success", false}}
        );
    }

    if (!email.contains("@"))
    {
        return common::Message::makeFailure(
            common::Command::SignupResult,
            common::ErrorCode::ValidationFailed,
            QStringLiteral("Invalid email format"),
            QJsonObject{{"success", false}}
        );
    }

    if (repo_.userExists(username))
    {
        return common::Message::makeFailure(
            common::Command::SignupResult,
            common::ErrorCode::AlreadyExists,
            QStringLiteral("Username already exists"),
            QJsonObject{{"success", false}}
        );
    }

    if (repo_.emailExists(email))
    {
        return common::Message::makeFailure(
            common::Command::SignupResult,
            common::ErrorCode::AlreadyExists,
            QStringLiteral("Email already exists"),
            QJsonObject{{"success", false}}
        );
    }

    User user;
    user.fullName     = fullName;
    user.username     = username;
    user.phone        = phone;
    user.email        = email;
    user.passwordHash = PasswordHasher::hash(password);
    user.role         = "User";

    try {
        repo_.createUser(user);
    } catch (const std::runtime_error&) {
        return common::Message::makeFailure(
            common::Command::SignupResult,
            common::ErrorCode::AlreadyExists,
            QStringLiteral("Username or email already exists"),
            QJsonObject{{"success", false}}
        );
    } catch (...) {
        return common::Message::makeFailure(
            common::Command::SignupResult,
            common::ErrorCode::InternalError,
            QStringLiteral("Signup failed due to server error"),
            QJsonObject{{"success", false}}
        );
    }

    return common::Message::makeSuccess(
        common::Command::SignupResult,
        QJsonObject{
            {"success", true}
        },
        {},
        {},
        QStringLiteral("Signup successful")
    );
}
