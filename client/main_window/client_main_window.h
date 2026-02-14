#ifndef KALANET_CLIENT_MAIN_WINDOW_H
#define KALANET_CLIENT_MAIN_WINDOW_H

#include <QMainWindow>
#include <QTimer>

class shop_page;
class cart_page;
class profile_page;
class new_ad_page;
class how_it_works_page;

QT_BEGIN_NAMESPACE
namespace Ui {
    class client_main_window;
}
QT_END_NAMESPACE

class client_main_window : public QMainWindow {
    Q_OBJECT

public:
    explicit client_main_window(QWidget *parent = nullptr);
    ~client_main_window() override;

    signals:
        // Clean navigation signals (you can connect these to stacked pages later)
        void newAdRequested();
    void shopRequested();
    void cartRequested();
    void profileRequested();
    void howItWorksRequested();

private slots:
    void updateTimeLabel();

    void on_btnRefreshTime_clicked();
    void on_btnNewAd_clicked();
    void on_btnShop_clicked();
    void on_btnCart_clicked();
    void on_btnProfile_clicked();
    void on_btnHowItWorks_clicked();
    void on_btnExit_clicked();

private:
    enum PageIndex {
        MenuPage = 0,
        ShopPage = 1,
        CartPage = 2,
        ProfilePage = 3,
        NewAdPage = 4,
        HowItWorksPage = 5
    };

    Ui::client_main_window *ui;
    QTimer *clockTimer;
    shop_page *shopPageWidget = nullptr;
    cart_page *cartPageWidget = nullptr;
    profile_page *profilePageWidget = nullptr;
    new_ad_page *newAdPageWidget = nullptr;
    how_it_works_page *howItWorksPageWidget = nullptr;


    void setupClock();
    void setupPages();
    void wireNavigation();
    void showPage(PageIndex page);
};

#endif // KALANET_CLIENT_MAIN_WINDOW_H
