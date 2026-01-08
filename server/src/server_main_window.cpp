#include "../headers/server_main_window.h"
#include "ui_server_main_window.h"
#include <QHostAddress>
#include <QMessageBox>
#include <QDebug>

server_main_window::server_main_window(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::server_main_window)
{
    ui->setupUi(this);

    server = new QTcpServer(this);

    // When a client connects, we’ll accept it in onNewConnection()
    connect(server, &QTcpServer::newConnection,
            this, &server_main_window::onNewConnection);
}

server_main_window::~server_main_window() {
    delete ui;
}

void server_main_window::on_startListeningButton_clicked()
{
    // If already listening, do nothing (or toggle stop if you want)
    if (server->isListening()) {
        QMessageBox::information(this, "Server", "Already listening.");
        return;
    }

    const QHostAddress addr = QHostAddress::AnyIPv4; // 0.0.0.0 (all interfaces)
    constexpr quint16 port = 4242;

    if (!server->listen(addr, port)) {
        QMessageBox::critical(this, "Listen failed", server->errorString());
        return;
    }

    QMessageBox::information(this, "Server",
                             QString("Listening on %1:%2")
                             .arg("0.0.0.0")
                             .arg(server->serverPort()));
    qInfo() << "Listening on" << server->serverAddress().toString() << server->serverPort();
}

void server_main_window::onNewConnection()
{
    while (server->hasPendingConnections()) {
        QTcpSocket *sock = server->nextPendingConnection();
        clients.insert(sock);

        qInfo() << "Client connected from"
                << sock->peerAddress().toString() << sock->peerPort();

        connect(sock, &QTcpSocket::readyRead, this, [sock]() {
            QByteArray data = sock->readAll();
            qInfo() << "RX:" << data << sock->peerAddress().toString() << sock->peerPort();
            sock->write("OK\n");
        });

        connect(sock, &QTcpSocket::disconnected, this, [this, sock]() {
            clients.remove(sock);
            sock->deleteLater();
            qInfo() << "Client disconnected: "
            << sock->peerAddress().toString() << sock->peerPort();;
        });
    }
}

void server_main_window::on_stopListeningButton_clicked()
{
    if (!server->isListening()) {
        QMessageBox::information(this, "Server", "Server is not listening.");
        return;
    }

    // 1. Stop accepting new connections
    server->close();

    // 2. Disconnect all active clients
    for (QTcpSocket* sock : std::as_const(clients)) {
        if (sock->state() == QAbstractSocket::ConnectedState) {
            sock->disconnectFromHost();
        }
        sock->deleteLater();
    }
    clients.clear();

    QMessageBox::information(this, "Server", "Server stopped listening.");
    qInfo() << "Server stopped.";
}
