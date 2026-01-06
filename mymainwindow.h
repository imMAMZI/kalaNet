#ifndef FINALPROJECT_MYMAINWINDOW_H
#define FINALPROJECT_MYMAINWINDOW_H

#include <QMainWindow>


QT_BEGIN_NAMESPACE

namespace Ui {
    class myMainWindow;
}

QT_END_NAMESPACE

class myMainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit myMainWindow(QWidget *parent = nullptr);

    ~myMainWindow() override;

private:
    Ui::myMainWindow *ui;
};


#endif //FINALPROJECT_MYMAINWINDOW_H