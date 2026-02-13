#include "signup_window.h"
#include "ui_signup_window.h"
#include "../network/auth_client.h"

#include <QRegularExpression>

#include "protocol/signup_message.h"


signup_window::signup_window(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::signup_window)
{
    ui->setupUi(this);
    clearError();

    connect(AuthClient::instance(), &AuthClient::signupResultReceived,
            this, [this](bool success, const QString& message) {
                ui->btnCreateAccount->setEnabled(true);

                if (!success) {
                    setError(message);
                    return;
                }

                clearError();
                emit signupSubmitted(
                    pendingName_,
                    pendingUsername_,
                    pendingPhone_,
                    pendingEmail_,
                    pendingPassword_
                );
            });

    connect(AuthClient::instance(), &AuthClient::networkError,
            this, [this](const QString& message) {
                ui->btnCreateAccount->setEnabled(true);
                setError(message);
            });
}

signup_window::~signup_window()
{
    delete ui;
}

void signup_window::on_btnCreateAccount_clicked()
{
    // TODO: implement later
    QString error;
    if (!validateInputs(error)) {
        setError(error);
        return;
    }

    pendingName_ = ui->leName->text().trimmed();
    pendingUsername_ = ui->leUsername->text().trimmed();
    pendingPhone_ = ui->lePhone->text().trimmed();
    pendingEmail_ = ui->leEmail->text().trimmed();
    pendingPassword_ = ui->lePassword->text();

    clearError();
    ui->btnCreateAccount->setEnabled(false);

    const common::Message request = common::SignupMessage::createRequest(
    pendingName_,
    pendingUsername_,
    pendingPhone_,
    pendingEmail_,
    pendingPassword_
);


    AuthClient::instance()->sendMessage(request);
}

void signup_window::on_btnBack_clicked()
{
    emit backToLoginRequested();
}

void signup_window::setError(const QString& message)
{
    ui->lblError->setText(message);
}

void signup_window::clearError()
{
    ui->lblError->clear();
}

bool signup_window::validateInputs(QString& errorOut) const
{
    const QString name = ui->leName->text().trimmed();
    const QString username = ui->leUsername->text().trimmed();
    const QString phone = ui->lePhone->text().trimmed();
    const QString email = ui->leEmail->text().trimmed();
    const QString password = ui->lePassword->text();
    const QString confirm = ui->leConfirmPassword->text();

    if (name.isEmpty() || username.isEmpty() || phone.isEmpty() || email.isEmpty() ||
        password.isEmpty() || confirm.isEmpty()) {
        errorOut = "All fields are required.";
        return false;
    }

    if (!isPhoneValid(phone)) {
        errorOut = "Phone number is not valid.";
        return false;
    }

    if (!isEmailValid(email)) {
        errorOut = "Email is not valid.";
        return false;
    }

    if (!isPasswordStrong(password)) {
        errorOut = "Password must be at least 8 chars and include upper/lower/digit.";
        return false;
    }

    if (password != confirm) {
        errorOut = "Password and confirmation do not match.";
        return false;
    }

    return true;
}

bool signup_window::isEmailValid(const QString& email) const
{
    static const QRegularExpression re(
        R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)"
    );
    return re.match(email).hasMatch();
}

bool signup_window::isPhoneValid(const QString& phone) const
{
    static const QRegularExpression re(R"(^\+?[0-9]{10,15}$)");
    return re.match(phone).hasMatch();
}

bool signup_window::isPasswordStrong(const QString& password) const
{
    if (password.size() < 8) {
        return false;
    }

    static const QRegularExpression upper(R"([A-Z])");
    static const QRegularExpression lower(R"([a-z])");
    static const QRegularExpression digit(R"([0-9])");

    return upper.match(password).hasMatch() &&
           lower.match(password).hasMatch() &&
           digit.match(password).hasMatch();
}

