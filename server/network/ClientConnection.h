#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include "../protocol/RequestDispatcher.h"

class ClientConnection : public QObject
{
    Q_OBJECT

public:
    explicit ClientConnection(QTcpSocket* socket, QObject* parent = nullptr);

    void send(const common::Message& message);

private slots:
    void onReadyRead();

private:
    QTcpSocket* socket_;
    RequestDispatcher dispatcher_;
};

#endif // CLIENT_CONNECTION_H
