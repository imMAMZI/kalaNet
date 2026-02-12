#include "ClientConnection.h"
#include "protocol/serializer.h"

ClientConnection::ClientConnection(QTcpSocket* socket, QObject* parent)
    : QObject(parent), socket_(socket)
{
    connect(socket_, &QTcpSocket::readyRead,
            this, &ClientConnection::onReadyRead);
}

void ClientConnection::onReadyRead()
{
    QByteArray rawData = socket_->readAll();

    common::Message message =
        common::Serializer::deserialize(rawData);

    dispatcher_.dispatch(message, *this);
}

void ClientConnection::send(const common::Message& message)
{
    QByteArray data = common::Serializer::serialize(message);
    socket_->write(data);
}
