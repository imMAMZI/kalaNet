//
// Created by hosse on 1/7/2026.
//

#include <QApplication>

#include "../headers/client_main_window.h"
#include "../headers/login_window.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    login_window login_page;
    client_main_window w;
    login_page.show();
    return QApplication::exec();
}