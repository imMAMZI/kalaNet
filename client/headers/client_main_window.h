//
// Created by hosse on 1/7/2026.
//

#ifndef KALANET_CLIENT_MAIN_WINDOW_H
#define KALANET_CLIENT_MAIN_WINDOW_H

#include <QWidget>


QT_BEGIN_NAMESPACE

namespace Ui
{
    class client_main_window;
}

QT_END_NAMESPACE

class client_main_window : public QWidget
{
    Q_OBJECT

public:
    explicit client_main_window(QWidget* parent = nullptr);
    ~client_main_window() override;

private:
    Ui::client_main_window* ui;

public slots:
    void myButtonClicked();
};


#endif //KALANET_CLIENT_MAIN_WINDOW_H