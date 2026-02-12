#include "TcpServer.h"
#include "ClientConnection.h"
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>

TcpServer::TcpServer(int port, RequestDispatcher& dispatcher, QObject* parent)
    : QObject(parent)
    , port_(port)
    , dispatcher_(dispatcher)
{
}

void TcpServer::start()
{
    auto* server = new QTcpServer(this);

    connect(server, &QTcpServer::newConnection, this, [this, server]() {
        while (server->hasPendingConnections()) {
            QTcpSocket* socket = server->nextPendingConnection();
            new ClientConnection(socket, dispatcher_, this);
        }
    });

    if (!server->listen(QHostAddress::Any, port_)) {
        qCritical() << "Server failed to listen on port" << port_;
    } else {
        qDebug() << "Server listening on port" << port_;
    }
}
