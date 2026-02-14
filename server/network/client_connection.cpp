#include "client_connection.h"
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

void ClientConnection::bindAuthenticatedIdentity(const QString& username,
                                                 const QString& role,
                                                 const QString& sessionToken)
{
    const QString previousUsername = authenticatedUsername_;
    authenticatedUsername_ = username.trimmed();
    authenticatedRole_ = role.trimmed();
    sessionToken_ = sessionToken.trimmed();
    if (previousUsername != authenticatedUsername_) {
        emit authenticatedUserChanged(previousUsername, authenticatedUsername_);
    }
}

void ClientConnection::updateAuthenticatedUsername(const QString& username)
{
    const QString previousUsername = authenticatedUsername_;
    authenticatedUsername_ = username.trimmed();
    if (previousUsername != authenticatedUsername_) {
        emit authenticatedUserChanged(previousUsername, authenticatedUsername_);
    }
}

void ClientConnection::clearAuthenticatedIdentity()
{
    const QString previousUsername = authenticatedUsername_;
    authenticatedUsername_.clear();
    authenticatedRole_.clear();
    sessionToken_.clear();
    if (!previousUsername.isEmpty()) {
        emit authenticatedUserChanged(previousUsername, QString());
    }
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
            common::Message response = common::Message::makeFailure(
                common::Command::Error,
                common::ErrorCode::InvalidJson,
                QStringLiteral("Invalid JSON format"),
                QJsonObject{}
            );
            sendResponse(common::Message(common::Command::Error), response);
            continue;
        }

        QString parseError;
        auto maybeMessage = common::Message::fromJson(doc.object(), &parseError);
        if (!maybeMessage) {
            const QString errorText = parseError.isEmpty()
                ? QStringLiteral("Malformed message envelope")
                : parseError;
            common::Message response = common::Message::makeFailure(
                common::Command::Error,
                common::ErrorCode::InvalidPayload,
                errorText,
                QJsonObject{}
            );
            sendResponse(common::Message(common::Command::Error), response);
            continue;
        }
        dispatcher_.dispatch(*maybeMessage, *this);
    }
}
void ClientConnection::sendResponse(const common::Message& request,
                                    const common::Message& response)
{
    send(response);                    // همان متد موجود برای ارسال روی سوکت
    emit requestProcessed(request, response);
}
