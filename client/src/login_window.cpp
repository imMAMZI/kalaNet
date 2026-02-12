#include "../headers/login_window.h"
#include "ui_login_window.h"
#include "../headers/signup_window.h"


login_window::login_window(QMainWindow *parent) : QMainWindow(parent), ui(new Ui::login_window) {
    ui->setupUi(this);
}

login_window::~login_window() {
    delete ui;
}

void login_window::on_btnLogin_clicked()
{
    // TODO: implement later (captcha + hashing + send to server)
}

void login_window::on_btnRefresh_clicked()
{
    // TODO: implement later (regenerate captcha)
}

void login_window::on_btnSignup_clicked()
{
    auto *dlg = new signup_window(this);  // parented => auto cleanup on close

    // When user clicks "Back", close dialog
    connect(dlg, &signup_window::backToLoginRequested, dlg, &QDialog::reject);

    // Optional: when signup submits successfully, close dialog and maybe fill login username
    connect(dlg, &signup_window::signupSubmitted, this,
            [this, dlg](const QString& name,
                        const QString& username,
                        const QString& phone,
                        const QString& email,
                        const QString& passPlain)
            {
                // later you’ll send this to server; for now just close and fill username
                ui->leUsername->setText(username);
                dlg->accept();
            });

    dlg->exec(); // modal popup
}
