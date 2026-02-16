#include "profile_page.h"
#include "ui_profile_page.h"

#include "../network/auth_client.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidgetItem>

profile_page::profile_page(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::profile_page)
{
    ui->setupUi(this);
    setupTables();

    connect(AuthClient::instance(), &AuthClient::profileHistoryReceived, this,
            [this](bool success, const QString& message, const QJsonObject& payload) {
                if (!success) {
                    showStatus(ui->lblProfileStatus, message);
                    return;
                }

                walletBalanceTokens = payload.value(QStringLiteral("walletBalanceTokens")).toInt(0);
                ui->lblBalanceValue->setText(QString::number(walletBalanceTokens) + " token");
                ui->leUsername->setText(payload.value(QStringLiteral("username")).toString());
                ui->leName->setText(payload.value(QStringLiteral("fullName")).toString());
                ui->lePhone->setText(payload.value(QStringLiteral("phone")).toString());
                ui->leEmail->setText(payload.value(QStringLiteral("email")).toString());

                purchases.clear();
                const QJsonArray purchasedAds = payload.value(QStringLiteral("purchasedAds")).toArray();
                for (const QJsonValue& value : purchasedAds) {
                    const QJsonObject ad = value.toObject();
                    purchases.push_back({ad.value(QStringLiteral("title")).toString(),
                                         ad.value(QStringLiteral("category")).toString(),
                                         ad.value(QStringLiteral("priceTokens")).toInt(0),
                                         ad.value(QStringLiteral("sellerUsername")).toString(),
                                         ad.value(QStringLiteral("createdAt")).toString().left(10)});
                }

                myAds.clear();
                const QJsonArray postedAds = payload.value(QStringLiteral("postedAds")).toArray();
                for (const QJsonValue& value : postedAds) {
                    const QJsonObject ad = value.toObject();
                    const QString status = ad.value(QStringLiteral("status")).toString().toLower();
                    if (status == QStringLiteral("sold")) {
                        continue;
                    }
                    myAds.push_back({ad.value(QStringLiteral("title")).toString(),
                                     ad.value(QStringLiteral("category")).toString(),
                                     ad.value(QStringLiteral("priceTokens")).toInt(0),
                                     status,
                                     0,
                                     ad.value(QStringLiteral("createdAt")).toString().left(10)});
                }

                const QJsonArray soldAds = payload.value(QStringLiteral("soldAds")).toArray();
                for (const QJsonValue& value : soldAds) {
                    const QJsonObject ad = value.toObject();
                    myAds.push_back({ad.value(QStringLiteral("title")).toString(),
                                     ad.value(QStringLiteral("category")).toString(),
                                     ad.value(QStringLiteral("priceTokens")).toInt(0),
                                     QStringLiteral("sold"),
                                     0,
                                     ad.value(QStringLiteral("updatedAt")).toString().left(10)});
                }

                refreshPurchasesTable();
                refreshMyAdsTable();
                showStatus(ui->lblProfileStatus, message);
            });

    connect(AuthClient::instance(), &AuthClient::walletBalanceReceived, this,
            [this](bool success, const QString&, int balanceTokens) {
                if (!success) {
                    return;
                }
                walletBalanceTokens = balanceTokens;
                ui->lblBalanceValue->setText(QString::number(walletBalanceTokens) + " token");
            });

    connect(AuthClient::instance(), &AuthClient::walletTopUpResultReceived, this,
            [this](bool success, const QString& message, int balanceTokens) {
                showStatus(ui->lblWalletStatus, message);
                if (!success) {
                    return;
                }
                walletBalanceTokens = balanceTokens;
                ui->lblBalanceValue->setText(QString::number(walletBalanceTokens) + " token");
                on_btnRefreshCaptcha_clicked();
            });

    connect(AuthClient::instance(), &AuthClient::captchaChallengeReceived, this,
            [this](bool success, const QString& message, const QString& scope,
                   const QString& challengeText, const QString& nonce, const QString&) {
                if (scope != QStringLiteral("wallet_top_up")) {
                    return;
                }
                if (!success) {
                    showStatus(ui->lblWalletStatus, message);
                    walletCaptchaNonce.clear();
                    ui->lblCaptchaText->setText("--");
                    return;
                }

                walletCaptchaNonce = nonce;
                ui->lblCaptchaText->setText(challengeText);
                ui->leCaptchaInput->clear();
                showStatus(ui->lblWalletStatus, QStringLiteral("Solve captcha then add tokens."));
            });

    connect(AuthClient::instance(), &AuthClient::profileUpdateResultReceived, this,
            [this](bool success, const QString& message, const QJsonObject&) {
                showStatus(ui->lblProfileStatus, message);
                if (passwordChangePending) {
                    showStatus(ui->lblPasswordStatus, message);
                    if (success) {
                        ui->leOldPassword->clear();
                        ui->leNewPassword->clear();
                        ui->leConfirmPassword->clear();
                    }
                    passwordChangePending = false;
                }
                if (success) {
                    refreshFromServer();
                }
            });

    connect(AuthClient::instance(), &AuthClient::adStatusNotifyReceived, this,
            [this](const QJsonArray&, const QString&) {
                refreshFromServer();
            });

    refreshFromServer();
    on_btnRefreshCaptcha_clicked();
}

profile_page::~profile_page()
{
    delete ui;
}

void profile_page::setupTables()
{
    if (ui->twPurchases) {
        ui->twPurchases->setColumnCount(5);
        ui->twPurchases->horizontalHeader()->setStretchLastSection(true);
        ui->twPurchases->verticalHeader()->setVisible(false);
        ui->twPurchases->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->twPurchases->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

    if (ui->twMyAds) {
        ui->twMyAds->setColumnCount(6);
        ui->twMyAds->horizontalHeader()->setStretchLastSection(true);
        ui->twMyAds->verticalHeader()->setVisible(false);
        ui->twMyAds->setSelectionBehavior(QAbstractItemView::SelectRows);
        ui->twMyAds->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

    ui->leUsername->setText(AuthClient::instance()->username());
    ui->leName->setText(AuthClient::instance()->fullName());
}

void profile_page::refreshFromServer()
{
    AuthClient* client = AuthClient::instance();
    client->sendMessage(client->withSession(common::Command::ProfileHistory,
                                                QJsonObject{{QStringLiteral("username"), client->username()}}));
    client->sendMessage(client->withSession(common::Command::WalletBalance));
}

void profile_page::refreshPurchasesTable()
{
    ui->twPurchases->setRowCount(static_cast<int>(purchases.size()));
    for (int row = 0; row < purchases.size(); ++row) {
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
    ui->twMyAds->setRowCount(static_cast<int>(myAds.size()));
    for (int row = 0; row < myAds.size(); ++row) {
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
    if (label) {
        label->setText(text);
    }
}

void profile_page::on_btnBackToMenu_clicked()
{
    emit backToMenuRequested();
}

void profile_page::on_btnSaveProfile_clicked()
{
    QJsonObject payload{{QStringLiteral("username"), ui->leUsername->text().trimmed()},
                        {QStringLiteral("fullName"), ui->leName->text().trimmed()},
                        {QStringLiteral("phone"), ui->lePhone->text().trimmed()},
                        {QStringLiteral("email"), ui->leEmail->text().trimmed()}};

    AuthClient::instance()->sendMessage(AuthClient::instance()->withSession(common::Command::ProfileUpdate, payload));
}

void profile_page::on_btnChangePassword_clicked()
{
    const QString oldPassword = ui->leOldPassword->text();
    const QString newPassword = ui->leNewPassword->text();
    const QString confirmPassword = ui->leConfirmPassword->text();

    if (oldPassword.trimmed().isEmpty()) {
        showStatus(ui->lblPasswordStatus, QStringLiteral("Old password is required."));
        return;
    }

    if (newPassword.trimmed().isEmpty()) {
        showStatus(ui->lblPasswordStatus, QStringLiteral("New password is required."));
        return;
    }

    if (newPassword != confirmPassword) {
        showStatus(ui->lblPasswordStatus, QStringLiteral("New password and confirm password do not match."));
        return;
    }

    QJsonObject payload{{QStringLiteral("username"), ui->leUsername->text().trimmed()},
                        {QStringLiteral("fullName"), ui->leName->text().trimmed()},
                        {QStringLiteral("phone"), ui->lePhone->text().trimmed()},
                        {QStringLiteral("email"), ui->leEmail->text().trimmed()},
                        {QStringLiteral("oldPassword"), oldPassword},
                        {QStringLiteral("password"), newPassword}};

    passwordChangePending = true;
    showStatus(ui->lblPasswordStatus, QStringLiteral("Updating password..."));
    AuthClient::instance()->sendMessage(AuthClient::instance()->withSession(common::Command::ProfileUpdate, payload));
}

void profile_page::on_btnRefreshCaptcha_clicked()
{
    AuthClient::instance()->sendMessage(common::Message(common::Command::CaptchaChallenge,
                                                        QJsonObject{{QStringLiteral("scope"), QStringLiteral("wallet_top_up")}}));
}

void profile_page::on_btnAddTokens_clicked()
{
    const int amount = ui->sbTokenAmount->value();
    bool ok = false;
    const int captchaAnswer = ui->leCaptchaInput->text().trimmed().toInt(&ok);

    if (amount <= 0 || !ok || walletCaptchaNonce.isEmpty()) {
        showStatus(ui->lblWalletStatus, QStringLiteral("Enter valid amount and captcha answer."));
        return;
    }

    QJsonObject payload{{QStringLiteral("amountTokens"), amount},
                        {QStringLiteral("captchaNonce"), walletCaptchaNonce},
                        {QStringLiteral("captchaAnswer"), captchaAnswer}};
    AuthClient::instance()->sendMessage(AuthClient::instance()->withSession(common::Command::WalletTopUp, payload));
}
