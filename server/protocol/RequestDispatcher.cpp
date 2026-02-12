#include "RequestDispatcher.h"
#include "../network/ClientConnection.h"
#include "protocol/commands.h"

RequestDispatcher::RequestDispatcher()
{
}

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
            common::Message response(
                common::Command::Error,
                QJsonObject{
                    {"message", "Unknown command"}
                }
            );
            client.send(response);
            break;
    }
    }
}

void RequestDispatcher::handleLogin(
    const common::Message& message,
    ClientConnection& client
) {
    common::Message response =
        authService_.login(message.payload());

    client.send(response);
}

void RequestDispatcher::handleSignup(
    const common::Message& message,
    ClientConnection& client
) {
    common::Message response =
        authService_.signup(message.payload());

    client.send(response);
}
