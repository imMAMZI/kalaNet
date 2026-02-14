#include "request_dispatcher.h"
#include "../network/client_connection.h"
#include "../ads/ad_service.h"
#include "../cart/cart_service.h"
#include "../wallet/wallet_service.h"
#include "protocol/commands.h"

#include <QJsonArray>
#include <utility>

RequestDispatcher::RequestDispatcher(AuthService& authService,
                                     AdService& adService,
                                     CartService& cartService,
                                     WalletService& walletService)
    : authService_(authService),
      adService_(adService),
      cartService_(cartService),
      walletService_(walletService)
{
}

void RequestDispatcher::setNotifyUserCallback(std::function<void(const QString&, const common::Message&)> callback)
{
    notifyUserCallback_ = std::move(callback);
}
// server/protocol/RequestDispatcher.cpp
void RequestDispatcher::dispatch(
    const common::Message& message,
    ClientConnection& client
) {
    switch (message.command()) {
    case common::Command::Login:
        handleLogin(message, client);
        break;

    case common::Command::Signup:
        handleSignup(message, client);
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

    default: {
            common::Message response = common::Message::makeFailure(
                common::Command::Error,
                common::ErrorCode::UnknownCommand,
                QStringLiteral("Unknown command"),
                QJsonObject{}
            );
            client.sendResponse(message, response);
            break;
    }
    }
}

void RequestDispatcher::handleLogin(
    const common::Message& message,
    ClientConnection& client
) {
    common::Message response = authService_.login(message.payload());
    client.sendResponse(message, response);
}

void RequestDispatcher::handleSignup(
    const common::Message& message,
    ClientConnection& client
) {
    common::Message response = authService_.signup(message.payload());
    client.sendResponse(message, response);
}

void RequestDispatcher::handleAdCreate(
    const common::Message& message,
    ClientConnection& client
) {
    common::Message response = adService_.create(message.payload());
    client.sendResponse(message, response);
}


void RequestDispatcher::handleAdList(
    const common::Message& message,
    ClientConnection& client
) {
    common::Message response = adService_.list(message.payload());
    client.sendResponse(message, response);
}

void RequestDispatcher::handleAdDetail(
    const common::Message& message,
    ClientConnection& client
) {
    common::Message response = adService_.detail(message.payload());
    client.sendResponse(message, response);
}


void RequestDispatcher::handleAdStatusUpdate(
    const common::Message& message,
    ClientConnection& client
) {
    common::Message response = adService_.updateStatus(message.payload());
    client.sendResponse(message, response);
}

void RequestDispatcher::handleCartAddItem(
    const common::Message& message,
    ClientConnection& client
)
{
    common::Message response = cartService_.addItem(message.payload());
    client.sendResponse(message, response);
}

void RequestDispatcher::handleCartRemoveItem(
    const common::Message& message,
    ClientConnection& client
)
{
    common::Message response = cartService_.removeItem(message.payload());
    client.sendResponse(message, response);
}

void RequestDispatcher::handleCartList(
    const common::Message& message,
    ClientConnection& client
)
{
    common::Message response = cartService_.list(message.payload());
    client.sendResponse(message, response);
}

void RequestDispatcher::handleCartClear(
    const common::Message& message,
    ClientConnection& client
)
{
    common::Message response = cartService_.clear(message.payload());
    client.sendResponse(message, response);
}

void RequestDispatcher::handleWalletBalance(const common::Message& message,
                                            ClientConnection& client)
{
    common::Message response = walletService_.walletBalance(message.payload());
    client.sendResponse(message, response);
}

void RequestDispatcher::handleWalletTopUp(const common::Message& message,
                                          ClientConnection& client)
{
    common::Message response = walletService_.walletTopUp(message.payload());
    client.sendResponse(message, response);

    if (response.isSuccess() && notifyUserCallback_) {
        const QString username = response.payload().value(QStringLiteral("username")).toString().trimmed();
        if (!username.isEmpty()) {
            common::Message notify(common::Command::WalletAdjustNotify, response.payload());
            notifyUserCallback_(username, notify);
        }
    }
}

void RequestDispatcher::handleBuy(const common::Message& message,
                                  ClientConnection& client)
{
    QSet<QString> affectedUsers;
    QVector<int> soldAdIds;
    common::Message response = walletService_.buy(message.payload(), &affectedUsers, &soldAdIds);
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
    common::Message response = walletService_.transactionHistory(message.payload());
    client.sendResponse(message, response);
}
