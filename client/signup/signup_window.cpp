//
// Created by mamzi on 2/12/26.
//

#include "signup_window.h"
#include "ui_signup_window.h"

signup_window::signup_window(QWidget *parent)
    : QDialog(parent),
      ui(new Ui::signup_window)
{
    ui->setupUi(this);
}

signup_window::~signup_window()
{
    delete ui;
}

void signup_window::on_btnCreateAccount_clicked()
{
    // TODO: implement later
}

void signup_window::on_btnBack_clicked()
{
    emit backToLoginRequested();
}

