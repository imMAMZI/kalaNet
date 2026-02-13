#include "auth_client.h"

#include <QCoreApplication>
#include <QDataStream>

#include "protocol/commands.h"
#include "protocol/serializer.h"

AuthClient* AuthClient::instance()
{
    static AuthClient* client = new AuthClient(QCoreApplication::instance());
    return client;
}

AuthClient::AuthClient(QObject* parent)
    : QObject(parent)
{
    connect(&socket_, &QTcpSocket::connected,
            this, &AuthClient::onConnected);

    connect(&socket_, &QTcpSocket::readyRead,
            this, &AuthClient::onReadyRead);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(&socket_, &QTcpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) {
                emit networkError(socket_.errorString());
            });
#else
    connect(&socket_, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, [this](QAbstractSocket::SocketError) {
                emit networkError(socket_.errorString());
            });
#endif
}

bool AuthClient::isConnected() const
{
    return socket_.state() == QAbstractSocket::ConnectedState;
}

void AuthClient::connectIfNeeded()
{
    if (socket_.state() == QAbstractSocket::ConnectedState ||
        socket_.state() == QAbstractSocket::ConnectingState) {
        return;
    }

    socket_.connectToHost(QString::fromUtf8(kHost), kPort);
}

void AuthClient::sendMessage(const common::Message& message)
{
    if (isConnected()) {
        sendFramed(message);
        return;
    }

    pendingMessages_.enqueue(message);
    connectIfNeeded();
}

void AuthClient::sendFramed(const common::Message& message)
{
    const QByteArray payload = common::Serializer::serialize(message);

    QByteArray frame;
    QDataStream out(&frame, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << static_cast<quint32>(payload.size());
    frame.append(payload);

    socket_.write(frame);
    socket_.flush();
}

void AuthClient::onConnected()
{
    while (!pendingMessages_.isEmpty()) {
        sendFramed(pendingMessages_.dequeue());
    }
}

void AuthClient::onReadyRead()
{
    buffer_.append(socket_.readAll());

    while (true) {
        if (expectedSize_ < 0) {
            if (buffer_.size() < 4) {
                return;
            }

            QByteArray lenBytes = buffer_.left(4);
            QDataStream in(lenBytes);
            in.setByteOrder(QDataStream::BigEndian);

            quint32 len = 0;
            in >> len;
            expectedSize_ = static_cast<qint32>(len);
            buffer_.remove(0, 4);
        }

        if (buffer_.size() < expectedSize_) {
            return;
        }

        const QByteArray payload = buffer_.left(expectedSize_);
        buffer_.remove(0, expectedSize_);
        expectedSize_ = -1;

        const common::Message message = common::Serializer::deserialize(payload);
        const QJsonObject data = message.payload();

        switch (message.command()) {
        case common::Command::LoginResult:
            emit loginResultReceived(
                data.value("success").toBool(false),
                data.value("message").toString(),
                data.value("fullName").toString(),
                data.value("role").toString()
            );
            break;

        case common::Command::SignupResult:
            emit signupResultReceived(
                data.value("success").toBool(false),
                data.value("message").toString()
            );
            break;

        case common::Command::Error:
            emit networkError(data.value("message").toString("Unknown protocol error"));
            break;

        default:
            emit networkError(QStringLiteral("Unexpected command from server"));
            break;
        }
    }
}