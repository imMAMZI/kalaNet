#ifndef KALANET_PENDINGADSWINDOW_H
#define KALANET_PENDINGADSWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class PendingAdsWindow; }
QT_END_NAMESPACE

class PendingAdsWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PendingAdsWindow(QWidget* parent = nullptr);
    ~PendingAdsWindow() override;

private:
    Ui::PendingAdsWindow* ui;
};

#endif // KALANET_PENDINGADSWINDOW_H
