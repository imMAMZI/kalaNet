//
// Created by hosse on 2/13/2026.
//

#ifndef KALANET_SERVERCONSOLEWINDOW_H
#define KALANET_SERVERCONSOLEWINDOW_H
// server/ui/serverconsolewindow.h

#include <QDateTime>
#include <QMainWindow>
#include <QSortFilterProxyModel>
#include <QTimer>
#include "RequestLogFilterProxy.h"

#include "requestlogmodel.h"
#include "protocol/message.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ServerConsoleWindow; }
QT_END_NAMESPACE

class ServerConsoleWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ServerConsoleWindow(QWidget* parent = nullptr);
    ~ServerConsoleWindow() override;

public slots:
    void onServerStarted(quint16 port);
    void onServerStopped();
    void onActiveConnectionCountChanged(int count);
    void onRequestLogged(const RequestLogEntry& entry);
    void onRequestProcessed(const common::Message& request,
                           const common::Message& response);

private slots:
    void applyFilters();
    void clearFilters();
    void clearLog();
    void exportLogs();
    void togglePause(bool paused);
    void updateUptime();

private:
    void setupConnections();
    void populateCommandFilter();
    void populateStatusFilter();

    QString mapCommandToText(common::Command command) const;
    QString mapErrorCodeToText(common::ErrorCode code) const;
    QString extractUsername(const QJsonObject& payload) const;

    Ui::ServerConsoleWindow* ui;
    RequestLogModel* logModel;
    RequestLogFilterProxy* proxyModel = nullptr;
    bool paused = false;
    QTimer uptimeTimer;
    QDateTime serverStartTime;
};

#endif //KALANET_SERVERCONSOLEWINDOW_H