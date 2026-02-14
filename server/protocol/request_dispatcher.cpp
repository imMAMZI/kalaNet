#include "request_dispatcher.h"
#include "../network/client_connection.h"
#include "../ads/ad_service.h"
#include "protocol/commands.h"

RequestDispatcher::RequestDispatcher(AuthService& authService,
                                     AdService& adService)
    : authService_(authService),
      adService_(adService)
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
