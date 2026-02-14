#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include "protocol/message.h"

class RequestDispatcher;

class ClientConnection : public QObject
{
    Q_OBJECT

signals:
    void requestProcessed(const common::Message& request,
                          const common::Message& response);
    void authenticatedUserChanged(const QString& previousUsername,
                                  const QString& currentUsername);

public:
    QString authenticatedUsername() const { return authenticatedUsername_; }
    QString authenticatedRole() const { return authenticatedRole_; }
    QString sessionToken() const { return sessionToken_; }
    void bindAuthenticatedIdentity(const QString& username,
                                   const QString& role,
                                   const QString& sessionToken);
    void updateAuthenticatedUsername(const QString& username);
    void clearAuthenticatedIdentity();

public:
    void sendResponse(const common::Message& request,
                      const common::Message& response);

public:
    explicit ClientConnection(QTcpSocket* socket,
                              RequestDispatcher& dispatcher,
                              QObject* parent = nullptr);

    void send(const common::Message& message);

private slots:
    void onReadyRead();

private:
    QTcpSocket* socket_;
    RequestDispatcher& dispatcher_;

    QByteArray buffer_;
    qint32 expectedSize_ = -1;
    QString authenticatedUsername_;
    QString authenticatedRole_;
    QString sessionToken_;
};

#endif // CLIENT_CONNECTION_H
