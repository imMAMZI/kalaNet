//
// Created by mamzi on 2/12/26.
//

#ifndef KALANET_SIGNUP_WINDOW_H
#define KALANET_SIGNUP_WINDOW_H

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {
    class signup_window;
}
QT_END_NAMESPACE

class signup_window : public QDialog {
    Q_OBJECT

public:
    explicit signup_window(QWidget *parent = nullptr);
    ~signup_window() override;

    signals:
    void backToLoginRequested();

    void signupSubmitted(const QString& name,
                         const QString& username,
                         const QString& phone,
                         const QString& email,
                         const QString& passwordPlain);

private slots:
    void on_btnCreateAccount_clicked();
    void on_btnBack_clicked();

private:
    void setError(const QString& message);
    void clearError();

    bool validateInputs(QString& errorOut) const;
    bool isEmailValid(const QString& email) const;
    bool isPhoneValid(const QString& phone) const;
    bool isPasswordStrong(const QString& password) const;

private:
    Ui::signup_window *ui;
    QString pendingName_;
    QString pendingUsername_;
    QString pendingPhone_;
    QString pendingEmail_;
    QString pendingPassword_;
};

#endif // KALANET_SIGNUP_WINDOW_H
