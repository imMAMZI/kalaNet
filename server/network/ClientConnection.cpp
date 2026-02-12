#include "ClientConnection.h"
#include "protocol/message.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDataStream>

ClientConnection::ClientConnection(QTcpSocket* socket,
                                   RequestDispatcher& dispatcher,
                                   QObject* parent)
    : QObject(parent)
    , socket_(socket)
    , dispatcher_(dispatcher)
{
    connect(socket_, &QTcpSocket::readyRead,
            this, &ClientConnection::onReadyRead);

    connect(socket_, &QTcpSocket::disconnected, this, [this]() {
        socket_->deleteLater();
        this->deleteLater();
    });
}

void ClientConnection::send(const common::Message& message)
{
    QJsonDocument doc(message.toJson());
    QByteArray payload = doc.toJson(QJsonDocument::Compact);

    QByteArray frame;
    QDataStream out(&frame, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << static_cast<quint32>(payload.size());
    frame.append(payload);

    socket_->write(frame);
    socket_->flush();
}

void ClientConnection::onReadyRead()
{
    buffer_.append(socket_->readAll());

    while (true) {
        if (expectedSize_ < 0) {
            if (buffer_.size() < 4)
                return;

            QByteArray lenBytes = buffer_.left(4);
            QDataStream in(lenBytes);
            in.setByteOrder(QDataStream::BigEndian);

            quint32 len = 0;
            in >> len;
            expectedSize_ = static_cast<qint32>(len);
            buffer_.remove(0, 4);
        }

        if (buffer_.size() < expectedSize_)
            return;

        QByteArray payload = buffer_.left(expectedSize_);
        buffer_.remove(0, expectedSize_);
        expectedSize_ = -1;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            common::Message response(
                common::Command::Error,
                QJsonObject{{"message", "Invalid JSON format"}}
            );
            send(response);
            continue;
        }

        common::Message message = common::Message::fromJson(doc.object());
        dispatcher_.dispatch(message, *this);
    }
}
