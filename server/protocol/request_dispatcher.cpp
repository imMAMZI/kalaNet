#include "request_dispatcher.h"
#include "../network/client_connection.h"
#include "protocol/commands.h"

RequestDispatcher::RequestDispatcher(AuthService& authService)
    : authService_(authService)
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
