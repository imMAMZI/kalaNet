#ifndef REQUEST_DISPATCHER_H
#define REQUEST_DISPATCHER_H

#include "protocol/message.h"
#include "../auth/auth_service.h"

class AdService;

class ClientConnection;

class RequestDispatcher
{
public:
    explicit RequestDispatcher(AuthService& authService,
                               AdService& adService);

    void dispatch(
        const common::Message& message,
        ClientConnection& client
    );

private:
    AuthService& authService_;
    AdService& adService_;

    void handleLogin(
        const common::Message& message,
        ClientConnection& client
    );

    void handleSignup(
        const common::Message& message,
        ClientConnection& client
    );

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
};

#endif // REQUEST_DISPATCHER_H
