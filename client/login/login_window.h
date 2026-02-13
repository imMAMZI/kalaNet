//
// Created by mamzi on 2/11/26.
//

#ifndef KALANET_LOGIN_WINDOW_H
#define KALANET_LOGIN_WINDOW_H

#include <QWidget>
#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
    class login_window;
}
QT_END_NAMESPACE

class client_main_window;

class login_window : public QMainWindow {
    Q_OBJECT

public:
    explicit login_window(QMainWindow *parent = nullptr);
    ~login_window() override;

    signals:
    void loginSucceeded(const QString& username);
    void signupRequested();

private slots:
    void on_btnLogin_clicked();
    void on_btnRefresh_clicked();
    void on_btnSignup_clicked();

private:
    void regenerateCaptcha();
    bool verifyCaptcha(const QString& input) const;
    QString hashPasswordSha256(const QString& password) const;

    void setError(const QString& message);
    void clearError();

private:
    Ui::login_window *ui;

    QString currentCaptcha;
    client_main_window* mainWindow_ = nullptr;
};

#endif // KALANET_LOGIN_WINDOW_H