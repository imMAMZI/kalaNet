#include <QApplication>
#include "../headers/mymainwindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    myMainWindow w;
    w.show();
    return QApplication::exec();
}