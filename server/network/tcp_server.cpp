#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>
#include <QMutexLocker>
#include "tcp_server.h"

#include "client_connection.h"
#include "protocol/commands.h"

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

    QSet<ClientConnection*> activeConnections;
    {
        QMutexLocker locker(&connectionsMutex_);
        activeConnections = connections_;
        connections_.clear();
        userConnections_.clear();
    }
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


void TcpServer::sendToUser(const QString& username, const common::Message& message)
{
    const QString normalized = username.trimmed();
    if (normalized.isEmpty()) {
        return;
    }

    QSet<ClientConnection*> connections;
    {
        QMutexLocker locker(&connectionsMutex_);
        connections = userConnections_.value(normalized);
    }
    for (ClientConnection* connection : connections) {
        if (connection) {
            connection->send(message);
        }
    }
}

void TcpServer::handleNewConnection()
{
    while (server_->hasPendingConnections()) {
        QTcpSocket* socket = server_->nextPendingConnection();
        auto* connection = new ClientConnection(socket, dispatcher_, this);
        int connectionCount = 0;
        {
            QMutexLocker locker(&connectionsMutex_);
            connections_.insert(connection);
            connectionCount = connections_.size();
        }
        emit activeConnectionCountChanged(connectionCount);

        connect(connection, &QObject::destroyed,
                this, &TcpServer::onConnectionDestroyed);
        connect(connection, &ClientConnection::requestProcessed,
                this, &TcpServer::requestProcessed);
        connect(connection, &ClientConnection::authenticatedUserChanged, this,
                [this, connection](const QString& previousUsername, const QString& currentUsername) {
            QMutexLocker locker(&connectionsMutex_);
            if (!previousUsername.isEmpty()) {
                userConnections_[previousUsername].remove(connection);
                if (userConnections_[previousUsername].isEmpty()) {
                    userConnections_.remove(previousUsername);
                }
            }
            if (!currentUsername.isEmpty()) {
                userConnections_[currentUsername].insert(connection);
            }
        });

    }
}

void TcpServer::onConnectionDestroyed(QObject* connection)
{
    auto* clientConnection = static_cast<ClientConnection*>(connection);
    int connectionCount = 0;
    bool removed = false;
    {
        QMutexLocker locker(&connectionsMutex_);
        if (clientConnection) {
            for (auto it = userConnections_.begin(); it != userConnections_.end(); ++it) {
                it.value().remove(clientConnection);
            }
        }

        removed = connections_.remove(clientConnection) > 0;
        connectionCount = connections_.size();
    }

    if (removed) {
        emit activeConnectionCountChanged(connectionCount);
    }
}
