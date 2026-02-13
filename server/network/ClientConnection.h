#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <QObject>
#include <QTcpSocket>
#include "../protocol/RequestDispatcher.h"

class ClientConnection : public QObject
{
    Q_OBJECT

    signals:
    void requestProcessed(const common::Message& request,
                          const common::Message& response);

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
};

#endif // CLIENT_CONNECTION_H
