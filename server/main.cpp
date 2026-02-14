#include <QApplication>
#include <QMessageBox>

#include "network/tcp_server.h"
#include "protocol/request_dispatcher.h"
#include "auth/auth_service.h"
#include "ads/ad_service.h"
#include "repository/sqlite_user_repository.h"
#include "ui/server_console_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ServerConsoleWindow console;
    console.show();

    static constexpr quint16 kDefaultServerPort = 8080;

    SqliteUserRepository userRepo("kalanet.db"); // مسیر فایل DB را مطابق پروژه تنظیم کنید
    AuthService authService(userRepo);
    AdService adService;
    RequestDispatcher dispatcher(authService, adService);

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
