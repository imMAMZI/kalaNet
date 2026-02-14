//
// Created by mamzi on 2/14/26.
//

#ifndef KALANET_PROFILE_PAGE_H
#define KALANET_PROFILE_PAGE_H

#include <QWidget>
#include <QString>
#include <QVector>
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class profile_page; }
QT_END_NAMESPACE

class profile_page : public QWidget {
    Q_OBJECT

public:
    explicit profile_page(QWidget *parent = nullptr);
    ~profile_page() override;

    // Later: you can call these from main window when user session loads
    void setUserBasicInfo(const QString& name,
                          const QString& username,
                          const QString& phone,
                          const QString& email);

    void setWalletBalance(int tokens);

    signals:
        void backToMenuRequested();

private slots:
    void on_btnBackToMenu_clicked();
    void on_btnSaveProfile_clicked();
    void on_btnChangePassword_clicked();
    void on_btnRefreshCaptcha_clicked();
    void on_btnAddTokens_clicked();

private:
    struct PurchaseRow {
        QString title;
        QString category;
        int priceTokens = 0;
        QString seller;
        QString date;
    };

    struct MyAdRow {
        QString title;
        QString category;
        int priceTokens = 0;
        QString status;   // Pending / Accepted / Rejected / Sold
        int views = 0;
        QString created;
    };

    void setupTables();
    void loadDummyData();

    QString generateCaptcha() const;
    void updateCaptchaUI(const QString& captcha);

    void refreshPurchasesTable();
    void refreshMyAdsTable();

    // local-only placeholder
    void showStatus(QLabel* label, const QString& text);

private:
    Ui::profile_page *ui;

    int walletBalanceTokens = 0;
    QString currentCaptcha;

    QVector<PurchaseRow> purchases;
    QVector<MyAdRow> myAds;
};

#endif // KALANET_PROFILE_PAGE_H
