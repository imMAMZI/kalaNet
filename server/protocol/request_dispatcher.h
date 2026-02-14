#ifndef REQUEST_DISPATCHER_H
#define REQUEST_DISPATCHER_H

#include "protocol/message.h"
#include "../auth/auth_service.h"

#include <functional>

class AdService;
class CartService;
class WalletService;

class ClientConnection;

class RequestDispatcher
{
public:
    explicit RequestDispatcher(AuthService& authService,
                               AdService& adService,
                               CartService& cartService,
                               WalletService& walletService);

    void setNotifyUserCallback(std::function<void(const QString&, const common::Message&)> callback);

    void dispatch(
        const common::Message& message,
        ClientConnection& client
    );

private:
    AuthService& authService_;
    AdService& adService_;
    CartService& cartService_;
    WalletService& walletService_;
    std::function<void(const QString&, const common::Message&)> notifyUserCallback_;

    void handleLogin(
        const common::Message& message,
        ClientConnection& client
    );

    void handleSignup(
        const common::Message& message,
        ClientConnection& client
    );

    void handleProfileUpdate(const common::Message& message, ClientConnection& client);
    void handleProfileHistory(const common::Message& message, ClientConnection& client);
    void handleAdminStats(const common::Message& message, ClientConnection& client);

    void handleAdCreate(
        const common::Message& message,
        ClientConnection& client
    );

    void handleAdList(
        const common::Message& message,
        ClientConnection& client
    );

    void handleAdDetail(
        const common::Message& message,
        ClientConnection& client
    );

    void handleAdStatusUpdate(
        const common::Message& message,
        ClientConnection& client
    );

    void handleCartAddItem(
        const common::Message& message,
        ClientConnection& client
    );

    void handleCartRemoveItem(
        const common::Message& message,
        ClientConnection& client
    );

    void handleCartList(
        const common::Message& message,
        ClientConnection& client
    );

    void handleCartClear(
        const common::Message& message,
        ClientConnection& client
    );

    void handleWalletBalance(
        const common::Message& message,
        ClientConnection& client
    );

    void handleWalletTopUp(
        const common::Message& message,
        ClientConnection& client
    );

    void handleBuy(
        const common::Message& message,
        ClientConnection& client
    );

    void handleTransactionHistory(
        const common::Message& message,
        ClientConnection& client
    );
};

#endif // REQUEST_DISPATCHER_H
