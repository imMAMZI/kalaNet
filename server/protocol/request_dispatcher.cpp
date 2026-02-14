#include "request_dispatcher.h"
#include "../network/client_connection.h"
#include "../ads/ad_service.h"
#include "../cart/cart_service.h"
#include "protocol/commands.h"

RequestDispatcher::RequestDispatcher(AuthService& authService,
                                     AdService& adService,
                                     CartService& cartService)
    : authService_(authService),
      adService_(adService),
      cartService_(cartService)
{
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
