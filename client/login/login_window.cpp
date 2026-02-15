#include "login_window.h"
#include "ui_login_window.h"
#include "../signup/signup_window.h"
#include "../network/auth_client.h"
#include "../main_window/client_main_window.h"

#include "protocol/login_message.h"

login_window::login_window(QMainWindow *parent)
    : QMainWindow(parent),
      ui(new Ui::login_window)
{
    ui->setupUi(this);

    connect(AuthClient::instance(), &AuthClient::loginResultReceived,
            this, [this](bool success,
                         const QString& message,
                         const QString&, const QString&) {
                ui->btnLogin->setEnabled(true);

                if (!success) {
                    setError(message);
                    requestCaptchaChallenge();
                    return;
                }

                clearError();
                if (!mainWindow_) {
                    mainWindow_ = new client_main_window();
                }
                mainWindow_->show();
                close();
            });

    connect(AuthClient::instance(), &AuthClient::networkError,
            this, [this](const QString& message) {
                ui->btnLogin->setEnabled(true);
                setError(message);
            });

    connect(AuthClient::instance(), &AuthClient::captchaChallengeReceived,
            this, [this](bool success, const QString& message, const QString& scope,
                         const QString& challengeText, const QString& nonce, const QString&) {
                if (scope != QStringLiteral("login")) {
                    return;
                }

                if (!success) {
                    setError(message);
                    ui->lblCaptchaText->setText(QStringLiteral("--"));
                    currentCaptchaNonce_.clear();
                    return;
                }

                currentCaptchaNonce_ = nonce;
                ui->lblCaptchaText->setText(challengeText);
                ui->leCaptcha->clear();
            });

    clearError();
    requestCaptchaChallenge();
}

login_window::~login_window()
{
    delete ui;
}

void login_window::on_btnLogin_clicked()
{
    const QString username = ui->leUsername->text().trimmed();
    const QString password = ui->lePassword->text();
    const QString captcha = ui->leCaptcha->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        setError("Username and password are required.");
        return;
    }

    const int captchaAnswer = captcha.toInt();
    if (currentCaptchaNonce_.isEmpty() || captcha.isEmpty() || QString::number(captchaAnswer) != captcha) {
        setError("Enter the numeric captcha answer.");
        return;
    }

    clearError();
    ui->btnLogin->setEnabled(false);

    const common::Message request = common::LoginMessage::createRequest(
        username.toStdString(),
        password.toStdString(),
        currentCaptchaNonce_,
        captchaAnswer
    );

    AuthClient::instance()->sendMessage(request);
}

void login_window::on_btnRefresh_clicked()
{
    clearError();
    requestCaptchaChallenge();
}

void login_window::on_btnSignup_clicked()
{
    signup_window dlg(this);

    connect(&dlg, &signup_window::backToLoginRequested, &dlg, &QDialog::reject);

    connect(&dlg, &signup_window::signupSubmitted, this,
            [this, &dlg](const QString&, const QString& username,
                         const QString&, const QString&, const QString& passPlain)
            {
                ui->leUsername->setText(username);
                ui->lePassword->setText(passPlain);
                dlg.accept();
            });

    dlg.exec();
}

void login_window::requestCaptchaChallenge()
{
    QJsonObject payload;
    payload.insert(QStringLiteral("scope"), QStringLiteral("login"));
    AuthClient::instance()->sendMessage(common::Message(common::Command::CaptchaChallenge, payload));
}

void login_window::setError(const QString& message)
{
    ui->lblError->setText(message);
}

void login_window::clearError()
{
    ui->lblError->clear();
}
