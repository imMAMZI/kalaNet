#include "../headers/login_window.h"
#include "ui_login_window.h"


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
    // TODO: implement later (open signup window/dialog)
}