#include <QApplication>
#include "../headers/server_main_window.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    server_main_window w;
    w.show();
    return QApplication::exec();
}