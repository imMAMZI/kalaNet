#include "../headers/mymainwindow.h"
#include "ui_mymainwindow.h"

#include <QHostAddress>
#include <QMessageBox>
#include <QDebug>

myMainWindow::myMainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::myMainWindow)
{
    ui->setupUi(this);

    server = new QTcpServer(this);

    // When a client connects, we’ll accept it in onNewConnection()
    connect(server, &QTcpServer::newConnection,
            this, &myMainWindow::onNewConnection);
}

myMainWindow::~myMainWindow() {
    delete ui;
}

void myMainWindow::on_pushButton_clicked()
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

void myMainWindow::onNewConnection()
{
    while (server->hasPendingConnections()) {
        QTcpSocket *sock = server->nextPendingConnection();

        qInfo() << "Client connected from"
                << sock->peerAddress().toString() << sock->peerPort();

        connect(sock, &QTcpSocket::readyRead, this, [sock]() {
            const QByteArray data = sock->readAll();
            qInfo() << "RX:" << data;
            sock->write("OK\n"); // example response
        });

        connect(sock, &QTcpSocket::disconnected, this, [sock]() {
            qInfo() << "Client disconnected";
            sock->deleteLater();
        });
    }
}
