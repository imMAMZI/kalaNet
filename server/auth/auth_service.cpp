#include "auth_service.h"
#include "protocol/commands.h"
#include "../security/captcha_service.h"
#include "../repository/ad_repository.h"
#include "../repository/wallet_repository.h"
#include "../logging_audit_logger.h"

#include <QJsonArray>
#include <stdexcept>
#include <limits>

AuthService::AuthService(UserRepository& repo,
                         CaptchaService& captchaService,
                         AdRepository* adRepository,
                         WalletRepository* walletRepository)
    : repo_(repo),
      adRepository_(adRepository),
      captchaService_(captchaService),
      walletRepository_(walletRepository)
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
    const QString captchaNonce = payload.value(QStringLiteral("captchaNonce")).toString().trimmed();
    const int captchaAnswer = payload.value(QStringLiteral("captchaAnswer")).toInt(std::numeric_limits<int>::min());

    if (username.isEmpty() || password.isEmpty()) {
        return common::Message::makeFailure(common::Command::LoginResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("Missing username or password"),
                                            QJsonObject{{"success", false}});
    }

    if (captchaNonce.isEmpty() || captchaAnswer == std::numeric_limits<int>::min()) {
        return common::Message::makeFailure(common::Command::LoginResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("CAPTCHA is required"),
                                            QJsonObject{{"success", false}});
    }

    QString captchaFailure;
    if (!captchaService_.verifyAndConsume(captchaNonce, captchaAnswer, QStringLiteral("login"), &captchaFailure)) {
        return common::Message::makeFailure(common::Command::LoginResult,
                                            common::ErrorCode::ValidationFailed,
                                            captchaFailure,
                                            QJsonObject{{"success", false}});
    }

    try {
        User user;
        if (!repo_.getUser(username, user)) {
            AuditLogger::log(QStringLiteral("auth.login"), QStringLiteral("failed"),
                             QJsonObject{{QStringLiteral("username"), username},
                                         {QStringLiteral("reason"), QStringLiteral("user_not_found")}});
            return common::Message::makeFailure(common::Command::LoginResult,
                                                common::ErrorCode::NotFound,
                                                QStringLiteral("User not found"),
                                                QJsonObject{{"success", false}});
        }

        if (!PasswordHasher::verify(password, user.passwordHash)) {
            AuditLogger::log(QStringLiteral("auth.login"), QStringLiteral("failed"),
                             QJsonObject{{QStringLiteral("username"), username},
                                         {QStringLiteral("reason"), QStringLiteral("invalid_credentials")}});
            return common::Message::makeFailure(common::Command::LoginResult,
                                                common::ErrorCode::AuthInvalidCredentials,
                                                QStringLiteral("Invalid username or password"),
                                                QJsonObject{{"success", false}});
        }

        AuditLogger::log(QStringLiteral("auth.login"), QStringLiteral("success"),
                         QJsonObject{{QStringLiteral("username"), username},
                                     {QStringLiteral("role"), roleToString(user.role)}});
        return common::Message::makeSuccess(common::Command::LoginResult,
                                            QJsonObject{{"success", true},
                                                        {"username", user.username},
                                                        {"fullName", user.fullName},
                                                        {"role", roleToString(user.role)}},
                                            {},
                                            {},
                                            QStringLiteral("Login successful"));
    } catch (const std::exception&) {
        return common::Message::makeFailure(common::Command::LoginResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("Login failed due to server error"),
                                            QJsonObject{{"success", false}});
    }
}

common::Message AuthService::signup(const QJsonObject& payload)
{
    const QString fullName = payload.value("fullName").toString();
    const QString username = payload.value("username").toString();
    const QString phone = payload.value("phone").toString();
    const QString email = payload.value("email").toString();
    const QString password = payload.value("password").toString();

    if (fullName.isEmpty() || username.isEmpty() || phone.isEmpty() || email.isEmpty() || password.isEmpty()) {
        return common::Message::makeFailure(common::Command::SignupResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("Missing required fields"),
                                            QJsonObject{{"success", false}});
    }

    if (password.size() < 8) {
        return common::Message::makeFailure(common::Command::SignupResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("Password must be at least 8 characters"),
                                            QJsonObject{{"success", false}});
    }

    if (!email.contains("@")) {
        return common::Message::makeFailure(common::Command::SignupResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("Invalid email format"),
                                            QJsonObject{{"success", false}});
    }

    if (repo_.userExists(username)) {
        return common::Message::makeFailure(common::Command::SignupResult,
                                            common::ErrorCode::AlreadyExists,
                                            QStringLiteral("Username already exists"),
                                            QJsonObject{{"success", false}});
    }

    if (repo_.emailExists(email)) {
        return common::Message::makeFailure(common::Command::SignupResult,
                                            common::ErrorCode::AlreadyExists,
                                            QStringLiteral("Email already exists"),
                                            QJsonObject{{"success", false}});
    }

    User user{fullName, username, phone, email, PasswordHasher::hash(password), QStringLiteral("User")};

    try {
        repo_.createUser(user);
    } catch (const std::runtime_error&) {
        return common::Message::makeFailure(common::Command::SignupResult,
                                            common::ErrorCode::AlreadyExists,
                                            QStringLiteral("Username or email already exists"),
                                            QJsonObject{{"success", false}});
    } catch (...) {
        return common::Message::makeFailure(common::Command::SignupResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("Signup failed due to server error"),
                                            QJsonObject{{"success", false}});
    }

    return common::Message::makeSuccess(common::Command::SignupResult,
                                        QJsonObject{{"success", true},
                                                    {"username", username}},
                                        {},
                                        {},
                                        QStringLiteral("Signup successful"));
}

common::Message AuthService::updateProfile(const QJsonObject& payload)
{
    const QString currentUsername = payload.value(QStringLiteral("currentUsername")).toString().trimmed();
    const QString newUsername = payload.value(QStringLiteral("username")).toString().trimmed();
    const QString fullName = payload.value(QStringLiteral("fullName")).toString().trimmed();
    const QString phone = payload.value(QStringLiteral("phone")).toString().trimmed();
    const QString email = payload.value(QStringLiteral("email")).toString().trimmed();
    const QString oldPassword = payload.value(QStringLiteral("oldPassword")).toString();
    const QString password = payload.value(QStringLiteral("password")).toString();

    if (currentUsername.isEmpty() || newUsername.isEmpty() || fullName.isEmpty() || phone.isEmpty() || email.isEmpty()) {
        return common::Message::makeFailure(common::Command::ProfileUpdateResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("Missing required profile fields"));
    }

    User existing;
    if (!repo_.getUser(currentUsername, existing)) {
        return common::Message::makeFailure(common::Command::ProfileUpdateResult,
                                            common::ErrorCode::NotFound,
                                            QStringLiteral("User not found"));
    }

    if (newUsername.compare(currentUsername, Qt::CaseInsensitive) != 0 && repo_.userExists(newUsername)) {
        return common::Message::makeFailure(common::Command::ProfileUpdateResult,
                                            common::ErrorCode::AlreadyExists,
                                            QStringLiteral("Username already exists"));
    }

    if (email.compare(existing.email, Qt::CaseInsensitive) != 0 && repo_.emailExists(email)) {
        return common::Message::makeFailure(common::Command::ProfileUpdateResult,
                                            common::ErrorCode::AlreadyExists,
                                            QStringLiteral("Email already exists"));
    }

    existing.username = newUsername;
    existing.fullName = fullName;
    existing.phone = phone;
    existing.email = email;
    if (!password.trimmed().isEmpty()) {
        if (oldPassword.trimmed().isEmpty()) {
            return common::Message::makeFailure(common::Command::ProfileUpdateResult,
                                                common::ErrorCode::ValidationFailed,
                                                QStringLiteral("Old password is required"));
        }

        if (!PasswordHasher::verify(oldPassword, existing.passwordHash)) {
            return common::Message::makeFailure(common::Command::ProfileUpdateResult,
                                                common::ErrorCode::ValidationFailed,
                                                QStringLiteral("Old password is incorrect"));
        }

        if (password.size() < 8) {
            return common::Message::makeFailure(common::Command::ProfileUpdateResult,
                                                common::ErrorCode::ValidationFailed,
                                                QStringLiteral("Password must be at least 8 characters"));
        }
        existing.passwordHash = PasswordHasher::hash(password);
    }

    try {
        if (!repo_.updateUser(currentUsername, existing)) {
            return common::Message::makeFailure(common::Command::ProfileUpdateResult,
                                                common::ErrorCode::AlreadyExists,
                                                QStringLiteral("Could not update profile due to duplicate username or email"));
        }
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(common::Command::ProfileUpdateResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("Profile update failed: %1").arg(ex.what()));
    }

    return common::Message::makeSuccess(common::Command::ProfileUpdateResult,
                                        QJsonObject{{QStringLiteral("username"), existing.username},
                                                    {QStringLiteral("email"), existing.email},
                                                    {QStringLiteral("fullName"), existing.fullName},
                                                    {QStringLiteral("phone"), existing.phone}},
                                        {},
                                        {},
                                        QStringLiteral("Profile updated successfully"));
}

common::Message AuthService::profileHistory(const QJsonObject& payload)
{
    if (!adRepository_ || !walletRepository_) {
        return common::Message::makeFailure(common::Command::ProfileHistoryResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("History services are unavailable"));
    }

    const QString username = payload.value(QStringLiteral("username")).toString().trimmed();
    const int limit = payload.value(QStringLiteral("limit")).toInt(50);
    if (username.isEmpty()) {
        return common::Message::makeFailure(common::Command::ProfileHistoryResult,
                                            common::ErrorCode::ValidationFailed,
                                            QStringLiteral("Username is required"));
    }

    try {
        const auto user = repo_.findByUsername(username);
        if (!user.has_value()) {
            return common::Message::makeFailure(common::Command::ProfileHistoryResult,
                                                common::ErrorCode::NotFound,
                                                QStringLiteral("User not found"));
        }

        const auto postedAds = adRepository_->listAdsBySeller(username, QString());
        const auto soldAds = adRepository_->listAdsBySeller(username, QStringLiteral("sold"));
        const auto purchasedAds = adRepository_->listPurchasedAdsByBuyer(username, limit);
        const auto tx = walletRepository_->transactionHistory(username, limit);
        const int balance = walletRepository_->getBalance(username);

        auto toJson = [](const AdRepository::AdSummaryRecord& ad) {
            return QJsonObject{{QStringLiteral("id"), ad.id},
                               {QStringLiteral("title"), ad.title},
                               {QStringLiteral("category"), ad.category},
                               {QStringLiteral("priceTokens"), ad.priceTokens},
                               {QStringLiteral("sellerUsername"), ad.sellerUsername},
                               {QStringLiteral("status"), ad.status},
                               {QStringLiteral("createdAt"), ad.createdAt},
                               {QStringLiteral("updatedAt"), ad.updatedAt},
                               {QStringLiteral("hasImage"), ad.hasImage}};
        };

        QJsonArray postedArray;
        for (const auto& ad : postedAds) postedArray.append(toJson(ad));
        QJsonArray soldArray;
        for (const auto& ad : soldAds) soldArray.append(toJson(ad));
        QJsonArray purchasedArray;
        for (const auto& ad : purchasedAds) purchasedArray.append(toJson(ad));

        QJsonArray txArray;
        QJsonArray walletChanges;
        for (const auto& entry : tx) {
            QJsonObject item{{QStringLiteral("id"), entry.id},
                             {QStringLiteral("type"), entry.type},
                             {QStringLiteral("amountTokens"), entry.amountTokens},
                             {QStringLiteral("balanceAfter"), entry.balanceAfter},
                             {QStringLiteral("adId"), entry.adId > 0 ? QJsonValue(entry.adId) : QJsonValue()},
                             {QStringLiteral("counterparty"), entry.counterparty},
                             {QStringLiteral("createdAt"), entry.createdAt.toString(Qt::ISODate)}};
            txArray.append(item);
            if (entry.amountTokens != 0) walletChanges.append(item);
        }

        QJsonObject response{{QStringLiteral("username"), username},
                             {QStringLiteral("fullName"), user->fullName},
                             {QStringLiteral("email"), user->email},
                             {QStringLiteral("phone"), user->phone},
                             {QStringLiteral("walletBalanceTokens"), balance},
                             {QStringLiteral("postedAds"), postedArray},
                             {QStringLiteral("soldAds"), soldArray},
                             {QStringLiteral("purchasedAds"), purchasedArray},
                             {QStringLiteral("walletChanges"), walletChanges},
                             {QStringLiteral("transactions"), txArray}};
        return common::Message::makeSuccess(common::Command::ProfileHistoryResult,
                                            response,
                                            {},
                                            {},
                                            QStringLiteral("Profile history loaded"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(common::Command::ProfileHistoryResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("Failed to load history: %1").arg(ex.what()));
    }
}

common::Message AuthService::adminStats(const QJsonObject&)
{
    if (!adRepository_) {
        return common::Message::makeFailure(common::Command::AdminStatsResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("Stats service unavailable"));
    }

    try {
        const int totalUsers = repo_.countAllUsers();
        const int adminUsers = repo_.countUsersByRole(QStringLiteral("Admin"));
        const int normalUsers = repo_.countUsersByRole(QStringLiteral("User"));
        const auto funnel = adRepository_->getAdStatusCounts();
        const auto sales = adRepository_->getSalesTotals();

        QJsonObject payload{{QStringLiteral("users"), QJsonObject{{QStringLiteral("total"), totalUsers},
                                                                     {QStringLiteral("admins"), adminUsers},
                                                                     {QStringLiteral("normalUsers"), normalUsers}}},
                            {QStringLiteral("adFunnel"), QJsonObject{{QStringLiteral("pending"), funnel.pending},
                                                                       {QStringLiteral("approved"), funnel.approved},
                                                                       {QStringLiteral("sold"), funnel.sold}}},
                            {QStringLiteral("sales"), QJsonObject{{QStringLiteral("soldAdsCount"), sales.soldAdsCount},
                                                                    {QStringLiteral("totalTokens"), sales.totalTokens}}}};

        return common::Message::makeSuccess(common::Command::AdminStatsResult,
                                            payload,
                                            {},
                                            {},
                                            QStringLiteral("Admin stats loaded"));
    } catch (const std::exception& ex) {
        return common::Message::makeFailure(common::Command::AdminStatsResult,
                                            common::ErrorCode::InternalError,
                                            QStringLiteral("Failed to load stats: %1").arg(ex.what()));
    }
}
