//
// Created by hosse on 1/7/2026.
//

#include <QApplication>

#include "../headers/client_main_window.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    client_main_window w;
    w.show();
    return QApplication::exec();
}