#ifndef FINALPROJECT_MYMAINWINDOW_H
#define FINALPROJECT_MYMAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class myMainWindow; }
QT_END_NAMESPACE

class myMainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit myMainWindow(QWidget *parent = nullptr);
    ~myMainWindow() override;

private slots:
    void on_pushButton_clicked();
    void onNewConnection();          // handles incoming connections

private:
    Ui::myMainWindow *ui;
    QTcpServer *server = nullptr;    // keep it alive for the lifetime of the window
};

#endif // FINALPROJECT_MYMAINWINDOW_H
