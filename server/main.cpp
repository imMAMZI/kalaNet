#include <QCoreApplication>
#include "network/TcpServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    TcpServer server(8080);
    server.start();

    return app.exec();
}
