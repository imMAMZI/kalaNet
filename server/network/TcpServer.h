#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <QObject>

#include <QObject>
#include <QHostAddress>
#include <QSet>

#include "protocol/message.h"

class QTcpServer;
class ClientConnection;
class RequestDispatcher;

class TcpServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpServer(quint16 port, RequestDispatcher& dispatcher, QObject* parent = nullptr);
    ~TcpServer() override;

    bool startListening(const QHostAddress& address = QHostAddress::Any);
    void stopListening();
    bool isListening() const;
    quint16 port() const noexcept { return port_; }

    signals:
        void serverStarted(quint16 port);
    void serverStopped();
    void activeConnectionCountChanged(int count);
    void requestProcessed(const common::Message& request,
                          const common::Message& response);

private:
    void handleNewConnection();
    void onConnectionDestroyed(QObject* connection);

    quint16 port_;
    RequestDispatcher& dispatcher_;
    QTcpServer* server_;
    QSet<ClientConnection*> connections_;
};

#endif // TCP_SERVER_H
