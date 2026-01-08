#include <QApplication>
#include "../headers/server_main_window.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    myMainWindow w;
    w.show();
    return QApplication::exec();
}