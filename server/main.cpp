#include "ui/serverconsolewindow.h"
// ...

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ServerConsoleWindow console;
    console.show();

    // اینجا instance سرور را ایجاد کنید
    TcpServer server;
    QObject::connect(&server, &TcpServer::serverStarted,
                     &console, &ServerConsoleWindow::onServerStarted);
    QObject::connect(&server, &TcpServer::serverStopped,
                     &console, &ServerConsoleWindow::onServerStopped);
    QObject::connect(&server, &TcpServer::activeConnectionCountChanged,
                     &console, &ServerConsoleWindow::onActiveConnectionCountChanged);
    QObject::connect(&server, &TcpServer::requestProcessed,
                     &console, &ServerConsoleWindow::onRequestLogged);

    if (!server.startListening()) {
        QMessageBox::critical(&console, QObject::tr("Server"),
                              QObject::tr("Failed to bind server socket"));
        return EXIT_FAILURE;
    }

    const int exitCode = app.exec();
    server.stopListening();
    return exitCode;
}
