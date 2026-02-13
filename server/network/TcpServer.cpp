#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include "TcpServer.h"

#include "ClientConnection.h"

TcpServer::TcpServer(quint16 port, RequestDispatcher& dispatcher, QObject* parent)
    : QObject(parent)
    , port_(port)
    , dispatcher_(dispatcher)
    , server_(new QTcpServer(this))
{
    connect(server_, &QTcpServer::newConnection,
            this, &TcpServer::handleNewConnection);
}

TcpServer::~TcpServer()
{
    stopListening();
}

bool TcpServer::startListening(const QHostAddress& address)
{
    if (server_->isListening()) {
        return true;
    }

    if (!server_->listen(address, port_)) {
        qCritical() << "Server failed to listen on port" << port_
                    << ':' << server_->errorString();
        return false;
    }

    port_ = server_->serverPort();
    emit serverStarted(port_);
    return true;
}

void TcpServer::stopListening()
{
    if (!server_->isListening()) {
        return;
    }

    server_->close();
    emit serverStopped();

    const auto activeConnections = connections_;
    connections_.clear();
    for (ClientConnection* connection : activeConnections) {
        if (connection) {
            connection->deleteLater();
        }
    }
    emit activeConnectionCountChanged(0);
}

bool TcpServer::isListening() const
{
    return server_->isListening();
}

void TcpServer::handleNewConnection()
{
    while (server_->hasPendingConnections()) {
        QTcpSocket* socket = server_->nextPendingConnection();
        auto* connection = new ClientConnection(socket, dispatcher_, this);
        connections_.insert(connection);
        emit activeConnectionCountChanged(connections_.size());

        connect(connection, &QObject::destroyed,
                this, &TcpServer::onConnectionDestroyed);
        connect(connection, &ClientConnection::requestProcessed,
        this, &TcpServer::requestProcessed);

    }
}

void TcpServer::onConnectionDestroyed(QObject* connection)
{
    auto* clientConnection = static_cast<ClientConnection*>(connection);
    if (connections_.remove(clientConnection) > 0) {
        emit activeConnectionCountChanged(connections_.size());
    }
}
