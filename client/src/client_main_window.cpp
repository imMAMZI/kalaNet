//
// Created by hosse on 1/7/2026.
//

// You may need to build the project (run Qt uic code generator) to get "ui_client_main_window.h" resolved

#include "../headers/client_main_window.h"
#include "ui_client_main_window.h"

#include <QHostAddress>
#include <QMessageBox>
#include <QDebug>

client_main_window::client_main_window(QWidget* parent)
    : QWidget(parent), ui(new Ui::client_main_window)
{
    ui->setupUi(this);

    socket_ = new QTcpSocket(this);

    connect(socket_, &QTcpSocket::connected, this, []() {
        qInfo() << "Connected to server!";
    });

    connect(socket_, &QTcpSocket::disconnected, this, []() {
        qInfo() << "Disconnected.";
    });

    connect(socket_, &QTcpSocket::readyRead, this, [this]() {
        const QByteArray data = socket_->readAll();
        qInfo() << "Server says:" << data;
    });

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(socket_, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        qWarning() << "Socket error:" << socket_->errorString();
    });
#else
    connect(socket_, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, [this](QAbstractSocket::SocketError) {
        qWarning() << "Socket error:" << socket_->errorString();
    });
#endif
}

client_main_window::~client_main_window()
{
    delete ui;
}

void client_main_window::on_pushButton_clicked()
{
    // If server is on the SAME machine, use 127.0.0.1
    // If server is on another machine, replace with its LAN IP (e.g. "192.168.1.10")
    const QString host = "127.0.0.1";
    constexpr quint16 port = 4242;

    // Avoid reconnect spam
    if (socket_->state() == QAbstractSocket::ConnectingState ||
        socket_->state() == QAbstractSocket::ConnectedState) {
        QMessageBox::information(this, "Client", "Already connecting/connected.");
        return;
        }

    socket_->connectToHost(host, port);

    // Optional: send something right after connecting
    // (better to send in connected() callback if you want guaranteed timing)
}
