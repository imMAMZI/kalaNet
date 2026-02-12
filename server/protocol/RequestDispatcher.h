#ifndef REQUEST_DISPATCHER_H
#define REQUEST_DISPATCHER_H

#include "protocol/message.h"
#include "../auth/AuthService.h"

class ClientConnection;

class RequestDispatcher
{
public:
    RequestDispatcher();

    void dispatch(
        const common::Message& message,
        ClientConnection& client
    );

private:
    AuthService authService_;

    void handleLogin(
        const common::Message& message,
        ClientConnection& client
    );

    void handleSignup(
        const common::Message& message,
        ClientConnection& client
    );
};

#endif // REQUEST_DISPATCHER_H
