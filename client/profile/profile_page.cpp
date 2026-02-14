//
// Created by mamzi on 2/14/26.
//

#include "profile_page.h"
#include "ui_profile_page.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QLabel>

profile_page::profile_page(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::profile_page)
{
    ui->setupUi(this);

    setupTables();
    loadDummyData();

    // Initial captcha
    currentCaptcha = generateCaptcha();
    updateCaptchaUI(currentCaptcha);
}

profile_page::~profile_page()
{
    delete ui;
}

void profile_page::setUserBasicInfo(const QString &name,
                                    const QString &username,
                                    const QString &phone,
                                    const QString &email)
{
    if (ui->leName) ui->leName->setText(name);
    if (ui->leUsername) ui->leUsername->setText(username);
    if (ui->lePhone) ui->lePhone->setText(phone);
    if (ui->leEmail) ui->leEmail->setText(email);
}

void profile_page::setWalletBalance(int tokens)
{
    walletBalanceTokens = tokens;
    if (ui->lblBalanceValue) ui->lblBalanceValue->setText(QString::number(tokens) + " token");
}

void profile_page::setupTables()
{
    // Purchases table
    if (ui->twPurchases) {
        ui->twPurchases->setColumnCount(5);
        ui->twPurchases->horizontalHeader()->setStretchLastSection(true);
        ui->twPurchases->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        ui->twPurchases->verticalHeader()->setVisible(false);

        ui->twPurchases->setColumnWidth(0, 240); // Title
        ui->twPurchases->setColumnWidth(1, 140); // Category
        ui->twPurchases->setColumnWidth(2, 110); // Price
        ui->twPurchases->setColumnWidth(3, 140); // Seller
        ui->twPurchases->setColumnWidth(4, 140); // Date

        ui->twPurchases->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->twPurchases->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->twPurchases->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

    // My Ads table
    if (ui->twMyAds) {
        ui->twMyAds->setColumnCount(6);
        ui->twMyAds->horizontalHeader()->setStretchLastSection(true);
        ui->twMyAds->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
        ui->twMyAds->verticalHeader()->setVisible(false);

        ui->twMyAds->setColumnWidth(0, 240); // Title
        ui->twMyAds->setColumnWidth(1, 140); // Category
        ui->twMyAds->setColumnWidth(2, 110); // Price
        ui->twMyAds->setColumnWidth(3, 120); // Status
        ui->twMyAds->setColumnWidth(4, 90);  // Views
        ui->twMyAds->setColumnWidth(5, 140); // Created

        ui->twMyAds->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->twMyAds->setSelectionMode(QAbstractItemView::SingleSelection);
        ui->twMyAds->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }
}

void profile_page::loadDummyData()
{
    // Basic info placeholders
    setUserBasicInfo("Mamzi User", "@mamzi", "09xxxxxxxxx", "mamzi@example.com");

    // Wallet placeholder
    setWalletBalance(200);

    // Purchases dummy rows
    purchases.clear();
    purchases.push_back({"C++ Book", "Books", 18, "Neda", "2026-02-14"});
    purchases.push_back({"Headphones", "Electronics", 55, "Mehdi", "2026-02-13"});

    // My ads dummy rows
    myAds.clear();
    myAds.push_back({"iPhone 11", "Electronics", 120, "Accepted", 34, "2026-02-10"});
    myAds.push_back({"Hoodie XL", "Clothing", 25, "Pending", 7, "2026-02-12"});
    myAds.push_back({"Old Desk", "Home", 40, "Rejected", 2, "2026-02-09"});

    refreshPurchasesTable();
    refreshMyAdsTable();
}

QString profile_page::generateCaptcha() const
{
    const QString chars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    QString cap;
    cap.reserve(5);

    for (int i = 0; i < 5; ++i) {
        int idx = QRandomGenerator::global()->bounded(chars.size());
        cap.append(chars[idx]);
    }
    return cap;
}

void profile_page::updateCaptchaUI(const QString &captcha)
{
    if (ui->lblCaptchaText) ui->lblCaptchaText->setText(captcha);
    if (ui->leCaptchaInput) ui->leCaptchaInput->clear();
}

void profile_page::refreshPurchasesTable()
{
    if (!ui->twPurchases) return;

    ui->twPurchases->setRowCount(static_cast<int>(purchases.size()));

    for (int row = 0; row < static_cast<int>(purchases.size()); ++row) {
        const auto &p = purchases[row];

        ui->twPurchases->setItem(row, 0, new QTableWidgetItem(p.title));
        ui->twPurchases->setItem(row, 1, new QTableWidgetItem(p.category));
        ui->twPurchases->setItem(row, 2, new QTableWidgetItem(QString::number(p.priceTokens) + " token"));
        ui->twPurchases->setItem(row, 3, new QTableWidgetItem(p.seller));
        ui->twPurchases->setItem(row, 4, new QTableWidgetItem(p.date));
    }
}

void profile_page::refreshMyAdsTable()
{
    if (!ui->twMyAds) return;

    ui->twMyAds->setRowCount(static_cast<int>(myAds.size()));

    for (int row = 0; row < static_cast<int>(myAds.size()); ++row) {
        const auto &a = myAds[row];

        ui->twMyAds->setItem(row, 0, new QTableWidgetItem(a.title));
        ui->twMyAds->setItem(row, 1, new QTableWidgetItem(a.category));
        ui->twMyAds->setItem(row, 2, new QTableWidgetItem(QString::number(a.priceTokens) + " token"));
        ui->twMyAds->setItem(row, 3, new QTableWidgetItem(a.status));
        ui->twMyAds->setItem(row, 4, new QTableWidgetItem(QString::number(a.views)));
        ui->twMyAds->setItem(row, 5, new QTableWidgetItem(a.created));
    }
}

void profile_page::showStatus(QLabel *label, const QString &text)
{
    if (!label) return;
    label->setText(text);
}

// -------------------- Slots (Qt auto-connect) --------------------

void profile_page::on_btnBackToMenu_clicked()
{
    emit backToMenuRequested();
}

void profile_page::on_btnSaveProfile_clicked()
{
    // Placeholder: just show a status message
    const QString name = ui->leName ? ui->leName->text().trimmed() : "";
    const QString username = ui->leUsername ? ui->leUsername->text().trimmed() : "";

    if (name.isEmpty() || username.isEmpty()) {
        showStatus(ui->lblProfileStatus, "Please fill at least Name and Username.");
        return;
    }

    showStatus(ui->lblProfileStatus, "Saved locally (server sync will be added later).");
}

void profile_page::on_btnChangePassword_clicked()
{
    // Placeholder: validate fields locally, real validation will be server-side
    const QString oldP = ui->leOldPassword ? ui->leOldPassword->text() : "";
    const QString newP = ui->leNewPassword ? ui->leNewPassword->text() : "";
    const QString conf = ui->leConfirmPassword ? ui->leConfirmPassword->text() : "";

    if (oldP.isEmpty() || newP.isEmpty() || conf.isEmpty()) {
        showStatus(ui->lblPasswordStatus, "Fill old/new/confirm password fields.");
        return;
    }
    if (newP != conf) {
        showStatus(ui->lblPasswordStatus, "New password and confirm password do not match.");
        return;
    }
    if (newP.size() < 4) {
        showStatus(ui->lblPasswordStatus, "Password too short (min 4).");
        return;
    }

    showStatus(ui->lblPasswordStatus, "Password change request prepared (server validation later).");
    if (ui->leOldPassword) ui->leOldPassword->clear();
    if (ui->leNewPassword) ui->leNewPassword->clear();
    if (ui->leConfirmPassword) ui->leConfirmPassword->clear();
}

void profile_page::on_btnRefreshCaptcha_clicked()
{
    currentCaptcha = generateCaptcha();
    updateCaptchaUI(currentCaptcha);
    showStatus(ui->lblWalletStatus, "Captcha refreshed.");
}

void profile_page::on_btnAddTokens_clicked()
{
    const int amount = ui->sbTokenAmount ? ui->sbTokenAmount->value() : 0;
    const QString typed = ui->leCaptchaInput ? ui->leCaptchaInput->text().trimmed() : "";

    if (amount <= 0) {
        showStatus(ui->lblWalletStatus, "Invalid amount.");
        return;
    }

    if (typed.compare(currentCaptcha, Qt::CaseInsensitive) != 0) {
        showStatus(ui->lblWalletStatus, "Captcha incorrect. Try again.");
        return;
    }

    walletBalanceTokens += amount;
    setWalletBalance(walletBalanceTokens);

    showStatus(ui->lblWalletStatus, "Tokens added locally (server validation later).");

    // new captcha after success
    currentCaptcha = generateCaptcha();
    updateCaptchaUI(currentCaptcha);
}
