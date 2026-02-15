#ifndef REQUEST_DISPATCHER_H
#define REQUEST_DISPATCHER_H

#include "protocol/message.h"
#include "../auth/auth_service.h"
#include "../auth/session_service.h"

#include <functional>
#include <optional>

class AdService;
class CartService;
class WalletService;
class ClientConnection;
class CaptchaService;

class RequestDispatcher
{
public:
    explicit RequestDispatcher(AuthService& authService,
                               SessionService& sessionService,
                               AdService& adService,
                               CartService& cartService,
                               WalletService& walletService,
                               CaptchaService& captchaService);

    void setNotifyUserCallback(std::function<void(const QString&, const common::Message&)> callback);

    void dispatch(const common::Message& message,
                  ClientConnection& client);

private:
    AuthService& authService_;
    SessionService& sessionService_;
    AdService& adService_;
    CartService& cartService_;
    WalletService& walletService_;
    CaptchaService& captchaService_;
    std::function<void(const QString&, const common::Message&)> notifyUserCallback_;

    std::optional<SessionService::SessionInfo> requireSession(const common::Message& message,
                                                              ClientConnection& client,
                                                              common::Command resultCommand,
                                                              const QString& requiredRole = {});

    void handleLogin(const common::Message& message,
                     ClientConnection& client);
    void handleSignup(const common::Message& message,
                      ClientConnection& client);
    void handleLogout(const common::Message& message,
                      ClientConnection& client);
    void handleSessionRefresh(const common::Message& message,
                              ClientConnection& client);

    void handleCaptchaChallenge(const common::Message& message, ClientConnection& client);
    void handleProfileUpdate(const common::Message& message, ClientConnection& client);
    void handleProfileHistory(const common::Message& message, ClientConnection& client);
    void handleAdminStats(const common::Message& message, ClientConnection& client);
    void handleAdCreate(const common::Message& message, ClientConnection& client);
    void handleAdList(const common::Message& message, ClientConnection& client);
    void handleAdDetail(const common::Message& message, ClientConnection& client);
    void handleAdStatusUpdate(const common::Message& message, ClientConnection& client);
    void handleCartAddItem(const common::Message& message, ClientConnection& client);
    void handleCartRemoveItem(const common::Message& message, ClientConnection& client);
    void handleCartList(const common::Message& message, ClientConnection& client);
    void handleCartClear(const common::Message& message, ClientConnection& client);
    void handleWalletBalance(const common::Message& message, ClientConnection& client);
    void handleWalletTopUp(const common::Message& message, ClientConnection& client);
    void handleBuy(const common::Message& message, ClientConnection& client);
    void handleDiscountCodeValidate(const common::Message& message, ClientConnection& client);
    void handleDiscountCodeList(const common::Message& message, ClientConnection& client);
    void handleDiscountCodeUpsert(const common::Message& message, ClientConnection& client);
    void handleDiscountCodeDelete(const common::Message& message, ClientConnection& client);
    void handleTransactionHistory(const common::Message& message, ClientConnection& client);
};

#endif // REQUEST_DISPATCHER_H
