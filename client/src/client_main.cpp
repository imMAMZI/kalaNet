//
// Created by hosse on 1/7/2026.
//

#include <QApplication>

#include "../main_window/client_main_window.h"
#include "../login/login_window.h"
#include "../main_window/client_main_window.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    client_main_window mw;
    login_window login_page;
    client_main_window w;
    mw.show();
    return QApplication::exec();
}