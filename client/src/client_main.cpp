//
// Created by hosse on 1/7/2026.
//

#include <QApplication>

#include "../login/login_window.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    login_window login_page;
    login_page.show();
    return QApplication::exec();
}
