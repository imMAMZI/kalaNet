//
// Created by hosse on 1/7/2026.
//

// You may need to build the project (run Qt uic code generator) to get "ui_client_main_window.h" resolved

#include "../headers/client_main_window.h"
#include "ui_client_main_window.h"


client_main_window::client_main_window(QWidget* parent) :
    QWidget(parent), ui(new Ui::client_main_window)
{
    ui->setupUi(this);
}

client_main_window::~client_main_window()
{
    delete ui;
}

void client_main_window::myButtonClicked()
{

}
