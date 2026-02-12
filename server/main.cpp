#include <QCoreApplication>

#include "network/TcpServer.h"
#include "protocol/RequestDispatcher.h"
#include "auth/AuthService.h"
#include "repository/SqliteUserRepository.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    SqliteUserRepository userRepo("kalanet.db");
    AuthService authService(userRepo);
    RequestDispatcher dispatcher(authService);

    TcpServer server(8080, dispatcher);
    server.start();

    return app.exec();
}
