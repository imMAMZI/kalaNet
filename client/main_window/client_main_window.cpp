#include "../main_window/client_main_window.h"
#include "ui_client_main_window.h"
#include "../cart/cart_page.h"
#include "../guide/how_it_works_page.h"
#include "../newAd/new_ad_page.h"
#include "../profile/profile_page.h"
#include "../shop/shop_page.h"
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
    setupPages();
    wireNavigation();
    updateTimeLabel();
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
void client_main_window::setupPages()
{
    auto clearLayout = [](QLayout *layout) {
        while (layout && layout->count() > 0) {
            QLayoutItem *item = layout->takeAt(0);
            if (!item) continue;
            if (QWidget *w = item->widget()) {
                w->deleteLater();
            }
            delete item;
        }
    };

    clearLayout(ui->pageShopLayout);
    clearLayout(ui->pageCartLayout);
    clearLayout(ui->pageProfileLayout);
    clearLayout(ui->pageNewAdLayout);
    clearLayout(ui->pageHowLayout);

    shopPageWidget = new shop_page(this);
    cartPageWidget = new cart_page(this);
    profilePageWidget = new profile_page(this);
    newAdPageWidget = new new_ad_page(this);
    howItWorksPageWidget = new how_it_works_page(this);

    ui->pageShopLayout->addWidget(shopPageWidget);
    ui->pageCartLayout->addWidget(cartPageWidget);
    ui->pageProfileLayout->addWidget(profilePageWidget);
    ui->pageNewAdLayout->addWidget(newAdPageWidget);
    ui->pageHowLayout->addWidget(howItWorksPageWidget);
}

void client_main_window::wireNavigation()
{
    connect(shopPageWidget, &shop_page::backToMenuRequested, this, [this]() { showPage(MenuPage); });
    connect(cartPageWidget, &cart_page::backToMenuRequested, this, [this]() { showPage(MenuPage); });
    connect(profilePageWidget, &profile_page::backToMenuRequested, this, [this]() { showPage(MenuPage); });
    connect(newAdPageWidget, &new_ad_page::backToMenuRequested, this, [this]() { showPage(MenuPage); });
    connect(howItWorksPageWidget, &how_it_works_page::backToMenuRequested, this, [this]() { showPage(MenuPage); });

    connect(shopPageWidget, &shop_page::goToCartRequested, this, [this]() { showPage(CartPage); });
    connect(cartPageWidget, &cart_page::purchaseRequested, this, [this](const QString &) { showPage(ProfilePage); });
}

void client_main_window::showPage(PageIndex page)
{
    ui->stackedMain->setCurrentIndex(page);
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
    showPage(NewAdPage);
}

void client_main_window::on_btnShop_clicked()
{
    emit shopRequested();
    showPage(ShopPage);
}

void client_main_window::on_btnCart_clicked()
{
    emit cartRequested();
    showPage(CartPage);
}

void client_main_window::on_btnProfile_clicked()
{
    emit profileRequested();
    showPage(ProfilePage);
}

void client_main_window::on_btnHowItWorks_clicked()
{
    emit howItWorksRequested();
    showPage(HowItWorksPage);
}

void client_main_window::on_btnExit_clicked()
{
    QApplication::quit();
}
