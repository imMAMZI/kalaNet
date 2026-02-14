#ifndef KALANET_AUTH_CLIENT_H
#define KALANET_AUTH_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QQueue>

#include "protocol/message.h"

class AuthClient : public QObject
{
    Q_OBJECT

public:
    static AuthClient* instance();

    void sendMessage(const common::Message& message);
    void connectIfNeeded();
    bool isConnected() const;

    signals:
        void loginResultReceived(bool success,
                                 const QString& message,
                                 const QString& fullName,
                                 const QString& role);

    void signupResultReceived(bool success,
                              const QString& message);

    void adCreateResultReceived(bool success,
                                const QString& message,
                                int adId);

    void networkError(const QString& message);

private:
    explicit AuthClient(QObject* parent = nullptr);

    void sendFramed(const common::Message& message);

private slots:
    void onConnected();
    void onReadyRead();

private:
    QTcpSocket socket_;
    QByteArray buffer_;
    qint32 expectedSize_ = -1;
    QQueue<common::Message> pendingMessages_;

    static constexpr quint16 kPort = 8080;
    static constexpr const char* kHost = "127.0.0.1";
};

#endif // KALANET_AUTH_CLIENT_H
