#include "request_dispatcher.h"

#include "../network/client_connection.h"
#include "../ads/ad_service.h"
#include "../cart/cart_service.h"
#include "../wallet/wallet_service.h"
#include "../security/captcha_service.h"
#include "protocol/commands.h"

#include <QJsonArray>
#include <utility>

RequestDispatcher::RequestDispatcher(AuthService& authService,
                                     SessionService& sessionService,
                                     AdService& adService,
                                     CartService& cartService,
                                     WalletService& walletService,
                                     CaptchaService& captchaService)
    : authService_(authService),
      sessionService_(sessionService),
      adService_(adService),
      cartService_(cartService),
      walletService_(walletService),
      captchaService_(captchaService)
{
}

void RequestDispatcher::setNotifyUserCallback(std::function<void(const QString&, const common::Message&)> callback)
{
    notifyUserCallback_ = std::move(callback);
}

std::optional<SessionService::SessionInfo> RequestDispatcher::requireSession(const common::Message& message,
                                                                              ClientConnection& client,
                                                                              common::Command resultCommand,
                                                                              const QString& requiredRole)
{
    const QString token = message.sessionToken().trimmed();
    if (token.isEmpty() || token != client.sessionToken()) {
        client.sendResponse(message, common::Message::makeFailure(resultCommand,
                                                                  common::ErrorCode::AuthSessionExpired,
                                                                  QStringLiteral("Authentication required")));
        return std::nullopt;
    }

    SessionService::SessionInfo session;
    if (!sessionService_.validateSession(token, &session)) {
        client.clearAuthenticatedIdentity();
        client.sendResponse(message, common::Message::makeFailure(resultCommand,
                                                                  common::ErrorCode::AuthSessionExpired,
                                                                  QStringLiteral("Session is invalid or expired")));
        return std::nullopt;
    }

    if (session.username != client.authenticatedUsername()) {
        client.sendResponse(message, common::Message::makeFailure(resultCommand,
                                                                  common::ErrorCode::AuthSessionExpired,
                                                                  QStringLiteral("Session does not match this socket identity")));
        return std::nullopt;
    }

    if (!requiredRole.trimmed().isEmpty() && session.role.compare(requiredRole, Qt::CaseInsensitive) != 0) {
        client.sendResponse(message, common::Message::makeFailure(resultCommand,
                                                                  common::ErrorCode::PermissionDenied,
                                                                  QStringLiteral("Permission denied")));
        return std::nullopt;
    }

    return session;
}

void RequestDispatcher::dispatch(const common::Message& message,
                                 ClientConnection& client)
{
    switch (message.command()) {
    case common::Command::Login:
        handleLogin(message, client);
        break;
    case common::Command::Signup:
        handleSignup(message, client);
        break;
    case common::Command::CaptchaChallenge:
        handleCaptchaChallenge(message, client);
        break;
    case common::Command::Logout:
        handleLogout(message, client);
        break;
    case common::Command::SessionRefresh:
        handleSessionRefresh(message, client);
        break;
    case common::Command::ProfileUpdate:
        handleProfileUpdate(message, client);
        break;
    case common::Command::ProfileHistory:
        handleProfileHistory(message, client);
        break;
    case common::Command::AdminStats:
        handleAdminStats(message, client);
        break;
    case common::Command::AdCreate:
        handleAdCreate(message, client);
        break;
    case common::Command::AdList:
        handleAdList(message, client);
        break;
    case common::Command::AdDetail:
        handleAdDetail(message, client);
        break;
    case common::Command::AdStatusUpdate:
        handleAdStatusUpdate(message, client);
        break;
    case common::Command::CartAddItem:
        handleCartAddItem(message, client);
        break;
    case common::Command::CartRemoveItem:
        handleCartRemoveItem(message, client);
        break;
    case common::Command::CartList:
        handleCartList(message, client);
        break;
    case common::Command::CartClear:
        handleCartClear(message, client);
        break;
    case common::Command::WalletBalance:
        handleWalletBalance(message, client);
        break;
    case common::Command::WalletTopUp:
        handleWalletTopUp(message, client);
        break;
    case common::Command::Buy:
        handleBuy(message, client);
        break;
    case common::Command::TransactionHistory:
        handleTransactionHistory(message, client);
        break;
    default:
        client.sendResponse(message, common::Message::makeFailure(common::Command::Error,
                                                                  common::ErrorCode::UnknownCommand,
                                                                  QStringLiteral("Unknown command"),
                                                                  QJsonObject{}));
        break;
    }
}

void RequestDispatcher::handleLogin(const common::Message& message,
                                    ClientConnection& client)
{
    common::Message response = authService_.login(message.payload());
    if (response.isSuccess()) {
        const QString username = response.payload().value(QStringLiteral("username")).toString().trimmed();
        const QString role = response.payload().value(QStringLiteral("role")).toString().trimmed();
        const QString token = sessionService_.createSession(username, role);

        if (username.isEmpty() || token.isEmpty()) {
            response = common::Message::makeFailure(common::Command::LoginResult,
                                                    common::ErrorCode::InternalError,
                                                    QStringLiteral("Failed to initialize authenticated session"));
        } else {
            QJsonObject payload = response.payload();
            payload.insert(QStringLiteral("sessionToken"), token);
            response.setPayload(payload);
            response.setSessionToken(token);
            client.bindAuthenticatedIdentity(username, role, token);
        }
    }
    client.sendResponse(message, response);
}

void RequestDispatcher::handleSignup(const common::Message& message,
                                     ClientConnection& client)
{
    common::Message response = authService_.signup(message.payload());
    client.sendResponse(message, response);
}

void RequestDispatcher::handleCaptchaChallenge(const common::Message& message,
                                               ClientConnection& client)
{
    const QString scope = message.payload().value(QStringLiteral("scope")).toString().trimmed().toLower();
    if (scope.isEmpty()) {
        client.sendResponse(message, common::Message::makeFailure(common::Command::CaptchaChallengeResult,
                                                                  common::ErrorCode::ValidationFailed,
                                                                  QStringLiteral("CAPTCHA scope is required")));
        return;
    }

    const auto challenge = captchaService_.createChallenge(scope);
    const QJsonObject payload{{QStringLiteral("scope"), challenge.scope},
                              {QStringLiteral("nonce"), challenge.nonce},
                              {QStringLiteral("challenge"), challenge.challengeText},
                              {QStringLiteral("expiresAt"), challenge.expiresAt.toString(Qt::ISODate)}};
    client.sendResponse(message, common::Message::makeSuccess(common::Command::CaptchaChallengeResult,
                                                              payload,
                                                              {},
                                                              {},
                                                              QStringLiteral("CAPTCHA challenge generated")));
}

void RequestDispatcher::handleLogout(const common::Message& message,
                                     ClientConnection& client)
{
    if (!requireSession(message, client, common::Command::LogoutResult).has_value()) {
        return;
    }

    sessionService_.invalidateSession(message.sessionToken());
    client.clearAuthenticatedIdentity();
    client.sendResponse(message, common::Message::makeSuccess(common::Command::LogoutResult,
                                                              QJsonObject{{QStringLiteral("success"), true}},
                                                              {},
                                                              {},
                                                              QStringLiteral("Logged out")));
}

void RequestDispatcher::handleSessionRefresh(const common::Message& message,
                                             ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::SessionRefreshResult);
    if (!session.has_value()) {
        return;
    }

    const QString newToken = sessionService_.refreshSession(message.sessionToken());
    if (newToken.isEmpty()) {
        client.sendResponse(message, common::Message::makeFailure(common::Command::SessionRefreshResult,
                                                                  common::ErrorCode::AuthSessionExpired,
                                                                  QStringLiteral("Failed to refresh session")));
        return;
    }

    client.bindAuthenticatedIdentity(session->username, session->role, newToken);
    client.sendResponse(message, common::Message::makeSuccess(common::Command::SessionRefreshResult,
                                                              QJsonObject{{QStringLiteral("username"), session->username},
                                                                          {QStringLiteral("sessionToken"), newToken}},
                                                              {},
                                                              newToken,
                                                              QStringLiteral("Session refreshed")));
}

void RequestDispatcher::handleProfileUpdate(const common::Message& message,
                                            ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::ProfileUpdateResult);
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    payload.insert(QStringLiteral("currentUsername"), session->username);

    common::Message response = authService_.updateProfile(payload);
    if (response.isSuccess()) {
        const QString updatedUsername = response.payload().value(QStringLiteral("username")).toString().trimmed();
        if (!updatedUsername.isEmpty() && updatedUsername.compare(session->username, Qt::CaseInsensitive) != 0) {
            sessionService_.updateSessionUsername(message.sessionToken(), updatedUsername);
            client.updateAuthenticatedUsername(updatedUsername);
        }
    }
    client.sendResponse(message, response);
}

void RequestDispatcher::handleProfileHistory(const common::Message& message,
                                             ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::ProfileHistoryResult);
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    payload.insert(QStringLiteral("username"), session->username);
    client.sendResponse(message, authService_.profileHistory(payload));
}

void RequestDispatcher::handleAdminStats(const common::Message& message,
                                         ClientConnection& client)
{
    if (!requireSession(message, client, common::Command::AdminStatsResult, QStringLiteral("Admin")).has_value()) {
        return;
    }

    client.sendResponse(message, authService_.adminStats(message.payload()));
}

void RequestDispatcher::handleAdCreate(const common::Message& message,
                                       ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::AdCreateResult);
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    payload.insert(QStringLiteral("sellerUsername"), session->username);
    client.sendResponse(message, adService_.create(payload));
}

void RequestDispatcher::handleAdList(const common::Message& message,
                                     ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::AdListResult);
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    if (session->role.compare(QStringLiteral("Admin"), Qt::CaseInsensitive) == 0) {
        payload.insert(QStringLiteral("allowAdminView"), true);
    }

    client.sendResponse(message, adService_.list(payload));
}

void RequestDispatcher::handleAdDetail(const common::Message& message,
                                       ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::AdDetailResult);
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    if (session->role.compare(QStringLiteral("Admin"), Qt::CaseInsensitive) == 0) {
        payload.insert(QStringLiteral("includeUnapproved"), true);
        payload.insert(QStringLiteral("includeHistory"), true);
    }

    client.sendResponse(message, adService_.detail(payload));
}

void RequestDispatcher::handleAdStatusUpdate(const common::Message& message,
                                             ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::AdStatusUpdate, QStringLiteral("Admin"));
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    payload.insert(QStringLiteral("moderatorUsername"), session->username);
    client.sendResponse(message, adService_.updateStatus(payload));
}

void RequestDispatcher::handleCartAddItem(const common::Message& message,
                                          ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::CartAddItemResult);
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    payload.insert(QStringLiteral("username"), session->username);
    client.sendResponse(message, cartService_.addItem(payload));
}

void RequestDispatcher::handleCartRemoveItem(const common::Message& message,
                                             ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::CartRemoveItemResult);
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    payload.insert(QStringLiteral("username"), session->username);
    client.sendResponse(message, cartService_.removeItem(payload));
}

void RequestDispatcher::handleCartList(const common::Message& message,
                                       ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::CartListResult);
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    payload.insert(QStringLiteral("username"), session->username);
    client.sendResponse(message, cartService_.list(payload));
}

void RequestDispatcher::handleCartClear(const common::Message& message,
                                        ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::CartClearResult);
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    payload.insert(QStringLiteral("username"), session->username);
    client.sendResponse(message, cartService_.clear(payload));
}

void RequestDispatcher::handleWalletBalance(const common::Message& message,
                                            ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::WalletBalanceResult);
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    payload.insert(QStringLiteral("username"), session->username);
    client.sendResponse(message, walletService_.walletBalance(payload));
}

void RequestDispatcher::handleWalletTopUp(const common::Message& message,
                                          ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::WalletTopUpResult);
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    payload.insert(QStringLiteral("username"), session->username);

    common::Message response = walletService_.walletTopUp(payload);
    client.sendResponse(message, response);

    if (response.isSuccess() && notifyUserCallback_) {
        common::Message notify(common::Command::WalletAdjustNotify, response.payload());
        notifyUserCallback_(session->username, notify);
    }
}

void RequestDispatcher::handleBuy(const common::Message& message,
                                  ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::BuyResult);
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    payload.insert(QStringLiteral("username"), session->username);

    QSet<QString> affectedUsers;
    QVector<int> soldAdIds;
    common::Message response = walletService_.buy(payload, &affectedUsers, &soldAdIds);
    client.sendResponse(message, response);

    if (response.isSuccess() && notifyUserCallback_) {
        for (const QString& username : std::as_const(affectedUsers)) {
            common::Message walletNotify(common::Command::WalletAdjustNotify, QJsonObject{{QStringLiteral("username"), username}});
            notifyUserCallback_(username, walletNotify);
        }

        if (!soldAdIds.isEmpty()) {
            QJsonArray adArray;
            for (const int adId : soldAdIds) {
                adArray.append(adId);
            }
            common::Message adNotify(common::Command::AdStatusNotify,
                                     QJsonObject{{QStringLiteral("soldAdIds"), adArray},
                                                 {QStringLiteral("status"), QStringLiteral("sold")}});
            for (const QString& username : std::as_const(affectedUsers)) {
                notifyUserCallback_(username, adNotify);
            }
        }
    }
}

void RequestDispatcher::handleTransactionHistory(const common::Message& message,
                                                 ClientConnection& client)
{
    const auto session = requireSession(message, client, common::Command::TransactionHistoryResult);
    if (!session.has_value()) {
        return;
    }

    QJsonObject payload = message.payload();
    payload.insert(QStringLiteral("username"), session->username);
    client.sendResponse(message, walletService_.transactionHistory(payload));
}
