#include "TcpServer.h"
#include <QTcpServer>
#include <QTcpSocket>
#include "ClientConnection.h"

TcpServer::TcpServer(int port, QObject* parent)
    : QObject(parent), port_(port)
{
}

void TcpServer::start()
{
    // Placeholder: TCP server واقعی بعداً
    qInfo("TcpServer started on port %d (symbolic)", port_);
}
