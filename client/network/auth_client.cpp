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
    connect(&socket_, &QTcpSocket::connected, this, &AuthClient::onConnected);
    connect(&socket_, &QTcpSocket::readyRead, this, &AuthClient::onReadyRead);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(&socket_, &QTcpSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError) { emit networkError(socket_.errorString()); });
#else
    connect(&socket_, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, [this](QAbstractSocket::SocketError) { emit networkError(socket_.errorString()); });
#endif
}

bool AuthClient::isConnected() const
{
    return socket_.state() == QAbstractSocket::ConnectedState;
}

QString AuthClient::sessionToken() const
{
    return sessionToken_;
}

QString AuthClient::username() const
{
    return username_;
}

QString AuthClient::fullName() const
{
    return fullName_;
}

common::Message AuthClient::withSession(common::Command command, const QJsonObject& payload, const QString& requestId) const
{
    return common::Message(command, payload, requestId, sessionToken_);
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

bool AuthClient::messageSuccess(const common::Message& message)
{
    return message.isSuccess() || message.payload().value(QStringLiteral("success")).toBool(false);
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

        const QByteArray payloadBytes = buffer_.left(expectedSize_);
        buffer_.remove(0, expectedSize_);
        expectedSize_ = -1;

        const common::Message message = common::Serializer::deserialize(payloadBytes);
        const QJsonObject payload = message.payload();
        const bool success = messageSuccess(message);
        const QString statusMessage = message.statusMessage().isEmpty()
                                          ? payload.value(QStringLiteral("message")).toString()
                                          : message.statusMessage();

        switch (message.command()) {
        case common::Command::LoginResult:
            if (success) {
                sessionToken_ = message.sessionToken().isEmpty()
                                    ? payload.value(QStringLiteral("sessionToken")).toString()
                                    : message.sessionToken();
                username_ = payload.value(QStringLiteral("username")).toString();
                fullName_ = payload.value(QStringLiteral("fullName")).toString();
            } else {
                sessionToken_.clear();
                username_.clear();
                fullName_.clear();
            }
            emit loginResultReceived(success,
                                     statusMessage,
                                     payload.value(QStringLiteral("fullName")).toString(),
                                     payload.value(QStringLiteral("role")).toString());
            break;

        case common::Command::SignupResult:
            emit signupResultReceived(success, statusMessage);
            break;

        case common::Command::CaptchaChallengeResult:
            emit captchaChallengeReceived(success,
                                         statusMessage,
                                         payload.value(QStringLiteral("scope")).toString(),
                                         payload.value(QStringLiteral("challenge")).toString(),
                                         payload.value(QStringLiteral("nonce")).toString(),
                                         payload.value(QStringLiteral("expiresAt")).toString());
            break;

        case common::Command::AdCreateResult:
            emit adCreateResultReceived(success, statusMessage, payload.value(QStringLiteral("adId")).toInt(-1));
            break;

        case common::Command::AdListResult:
            emit adListReceived(success, statusMessage, payload.value(QStringLiteral("ads")).toArray());
            break;

        case common::Command::AdDetailResult:
            emit adDetailResultReceived(success, statusMessage, payload);
            break;

        case common::Command::CartListResult:
            emit cartListReceived(success, statusMessage, payload.value(QStringLiteral("items")).toArray());
            break;

        case common::Command::CartAddItemResult:
            emit cartAddItemResultReceived(success,
                                           statusMessage,
                                           payload.value(QStringLiteral("adId")).toInt(-1),
                                           payload.value(QStringLiteral("added")).toBool(false));
            break;

        case common::Command::CartRemoveItemResult:
            emit cartRemoveItemResultReceived(success, statusMessage, payload.value(QStringLiteral("adId")).toInt(-1));
            break;

        case common::Command::CartClearResult:
            emit cartClearResultReceived(success, statusMessage);
            break;

        case common::Command::WalletBalanceResult:
            emit walletBalanceReceived(success, statusMessage, payload.value(QStringLiteral("balanceTokens")).toInt(0));
            break;

        case common::Command::WalletTopUpResult:
            emit walletTopUpResultReceived(success, statusMessage, payload.value(QStringLiteral("balanceTokens")).toInt(0));
            break;

        case common::Command::BuyResult:
            emit buyResultReceived(success,
                                  statusMessage,
                                  payload.value(QStringLiteral("balanceTokens")).toInt(0),
                                  payload.value(QStringLiteral("soldAdIds")).toArray());
            break;

        case common::Command::ProfileHistoryResult:
            emit profileHistoryReceived(success, statusMessage, payload);
            break;

        case common::Command::ProfileUpdateResult:
            emit profileUpdateResultReceived(success, statusMessage, payload);
            break;

        case common::Command::WalletAdjustNotify:
            sendMessage(withSession(common::Command::WalletBalance));
            break;

        case common::Command::AdStatusNotify:
            emit adStatusNotifyReceived(payload.value(QStringLiteral("soldAdIds")).toArray(),
                                        payload.value(QStringLiteral("status")).toString());
            break;

        case common::Command::Error:
            emit networkError(statusMessage.isEmpty() ? QStringLiteral("Unknown protocol error") : statusMessage);
            break;

        default:
            break;
        }
    }
}
