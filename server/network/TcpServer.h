#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <QObject>

class TcpServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpServer(int port, QObject* parent = nullptr);
    void start();

private:
    int port_;
};

#endif // TCP_SERVER_H
