#include "pending_ads_window.h"

#include "ui_pending_ads_window.h"

PendingAdsWindow::PendingAdsWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::PendingAdsWindow)
{
    ui->setupUi(this);
}

PendingAdsWindow::~PendingAdsWindow()
{
    delete ui;
}
