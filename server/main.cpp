#include <QApplication>
#include <QMessageBox>

#include "network/TcpServer.h"
#include "protocol/RequestDispatcher.h"
#include "auth/AuthService.h"
#include "repository/SqliteUserRepository.h"
#include "ui/ServerConsoleWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ServerConsoleWindow console;
    console.show();

    static constexpr quint16 kDefaultServerPort = 8080;

    SqliteUserRepository userRepo("kalanet.db"); // مسیر فایل DB را مطابق پروژه تنظیم کنید
    AuthService authService(userRepo);
    RequestDispatcher dispatcher(authService);

    TcpServer server(kDefaultServerPort, dispatcher);

    QObject::connect(&server, &TcpServer::serverStarted,
                     &console, &ServerConsoleWindow::onServerStarted);

    QObject::connect(&server, &TcpServer::serverStopped,
                     &console, &ServerConsoleWindow::onServerStopped);

    QObject::connect(&server, &TcpServer::activeConnectionCountChanged,
                     &console, &ServerConsoleWindow::onActiveConnectionCountChanged);

    QObject::connect(&server, &TcpServer::requestProcessed,
                     &console, &ServerConsoleWindow::onRequestProcessed);


    if (!server.startListening()) {
        QMessageBox::critical(&console, QObject::tr("Server"),
                              QObject::tr("Failed to bind server socket"));
        return EXIT_FAILURE;
    }

    const int exitCode = app.exec();
    server.stopListening();
    return exitCode;
}
