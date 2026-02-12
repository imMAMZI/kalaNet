#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <QObject>

class RequestDispatcher;

class TcpServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpServer(int port, RequestDispatcher& dispatcher, QObject* parent = nullptr);
    void start();

private:
    int port_;
    RequestDispatcher& dispatcher_;
};

#endif // TCP_SERVER_H
