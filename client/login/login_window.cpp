#include "login_window.h"
#include "ui_login_window.h"
#include "../signup/signup_window.h"
#include "../network/auth_client.h"
#include "../headers/client_main_window.h"

#include <QCryptographicHash>
#include <QRandomGenerator>

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
                    regenerateCaptcha();
                    ui->leCaptcha->clear();
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

    regenerateCaptcha();
    clearError();
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

    if (!verifyCaptcha(captcha)) {
        setError("Captcha is incorrect.");
        regenerateCaptcha();
        ui->leCaptcha->clear();
        return;
    }

    clearError();
    ui->btnLogin->setEnabled(false);

    const common::Message request = common::LoginMessage::createRequest(
        username.toStdString(),
        password.toStdString()
    );

    AuthClient::instance()->sendMessage(request);
}

void login_window::on_btnRefresh_clicked()
{
    regenerateCaptcha();
    ui->leCaptcha->clear();
    clearError();
}

void login_window::on_btnSignup_clicked()
{
    auto *dlg = new signup_window(this);

    connect(dlg, &signup_window::backToLoginRequested, dlg, &QDialog::reject);

    connect(dlg, &signup_window::signupSubmitted, this,
    [this, dlg](const QString&, const QString& username,
                const QString&, const QString&, const QString& passPlain)
            {
                ui->leUsername->setText(username);
                ui->lePassword->setText(passPlain);
                dlg->accept();
            });

    dlg->exec();
}

void login_window::regenerateCaptcha()
{
    static constexpr char kCharset[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    QString generated;
    generated.reserve(6);

    for (int i = 0; i < 6; ++i) {
        const int idx = QRandomGenerator::global()->bounded(static_cast<int>(sizeof(kCharset) - 1));
        generated.append(QChar(kCharset[idx]));
    }

    currentCaptcha = generated;
    ui->lblCaptchaText->setText(currentCaptcha);
}

bool login_window::verifyCaptcha(const QString& input) const
{
    return input.trimmed().compare(currentCaptcha, Qt::CaseInsensitive) == 0;
}

QString login_window::hashPasswordSha256(const QString& password) const
{
    const QByteArray digest = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromUtf8(digest.toHex());
}

void login_window::setError(const QString& message)
{
    ui->lblError->setText(message);
}

void login_window::clearError()
{
    ui->lblError->clear();
}