#include "../main_window/client_main_window.h"
#include "ui_client_main_window.h"

#include <QDateTime>
#include <QApplication>
#include <QMessageBox>

client_main_window::client_main_window(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::client_main_window),
      clockTimer(new QTimer(this))
{
    ui->setupUi(this);

    setupClock();
    updateTimeLabel(); // initial update immediately
}

client_main_window::~client_main_window()
{
    delete ui;
}

void client_main_window::setupClock()
{
    // Update time every second
    clockTimer->setInterval(1000);
    connect(clockTimer, &QTimer::timeout, this, &client_main_window::updateTimeLabel);
    clockTimer->start();
}

void client_main_window::updateTimeLabel()
{
    // Format: HH:MM:SS (24-hour)
    const QString now = QDateTime::currentDateTime().toString("HH:mm:ss");
    if (ui->lblTime) {
        ui->lblTime->setText(now);
    }
}

// ---------- Button slots ----------

void client_main_window::on_btnRefreshTime_clicked()
{
    updateTimeLabel();
}

void client_main_window::on_btnNewAd_clicked()
{
    emit newAdRequested();

    // Temporary placeholder until we build the page:
    QMessageBox::information(this, "New Ad", "New Ad page is not implemented yet.");
}

void client_main_window::on_btnShop_clicked()
{
    emit shopRequested();
    QMessageBox::information(this, "Shop", "Shop page is not implemented yet.");
}

void client_main_window::on_btnCart_clicked()
{
    emit cartRequested();
    QMessageBox::information(this, "Cart", "Cart page is not implemented yet.");
}

void client_main_window::on_btnProfile_clicked()
{
    emit profileRequested();
    QMessageBox::information(this, "Profile", "Profile page is not implemented yet.");
}

void client_main_window::on_btnHowItWorks_clicked()
{
    emit howItWorksRequested();
    QMessageBox::information(this, "How does it work?", "How it works page is not implemented yet.");
}

void client_main_window::on_btnExit_clicked()
{
    // “Kills the program”
    // close() is enough, but this ensures app exits even if other windows exist.
    QApplication::quit();
}
