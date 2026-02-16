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
    struct PurchaseRow {
        QString title;
        QString category;
        int priceTokens = 0;
        QString seller;
        QString date;
    };
    void refreshFromServer();


    explicit profile_page(QWidget *parent = nullptr);
    ~profile_page() override;

private slots:
    void on_btnBackToMenu_clicked();
    void on_btnSaveProfile_clicked();
    void on_btnChangePassword_clicked();
    void on_btnRefreshCaptcha_clicked();
    void on_btnAddTokens_clicked();

signals:
    void backToMenuRequested();

private:
    struct MyAdRow {
        QString title;
        QString category;
        int priceTokens = 0;
        QString status;
        int views = 0;
        QString created;
    };

    void setupTables();
    void refreshPurchasesTable();
    void refreshMyAdsTable();
    void showStatus(QLabel* label, const QString& text);

private:
    Ui::profile_page *ui;

    int walletBalanceTokens = 0;
    QString walletCaptchaNonce;

    QVector<PurchaseRow> purchases;
    QVector<MyAdRow> myAds;
    bool passwordChangePending = false;
};

#endif // KALANET_PROFILE_PAGE_H
