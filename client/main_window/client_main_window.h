#ifndef KALANET_CLIENT_MAIN_WINDOW_H
#define KALANET_CLIENT_MAIN_WINDOW_H

#include <QMainWindow>
#include <QTimer>

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
    Ui::client_main_window *ui;
    QTimer *clockTimer;

    void setupClock();
};

#endif // KALANET_CLIENT_MAIN_WINDOW_H
