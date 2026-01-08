#ifndef FINALPROJECT_MYMAINWINDOW_H
#define FINALPROJECT_MYMAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSet>

QT_BEGIN_NAMESPACE
namespace Ui { class server_main_window; }
QT_END_NAMESPACE

class server_main_window : public QMainWindow {
    Q_OBJECT

public:
    explicit server_main_window(QWidget *parent = nullptr);
    ~server_main_window() override;

private slots:
    void on_startListeningButton_clicked();
    void on_stopListeningButton_clicked();
    void onNewConnection();          // handles incoming connections

private:
    Ui::server_main_window *ui;
    QTcpServer *server = nullptr;    // keep it alive for the lifetime of the window
    QSet<QTcpSocket*> clients;
};

#endif // FINALPROJECT_MYMAINWINDOW_H
