// server/ui/serverconsolewindow.cpp
#include <QApplication>

#include "server_console_window.h"
#include "ui_server_console_window.h"
#include "request_log_model.h"
#include "request_log_filter_proxy.h"

#include "pending_ads_window.h"
#include "protocol/error_codes.h"
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QTextStream>

ServerConsoleWindow::ServerConsoleWindow(AdRepository& adRepository, QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::ServerConsoleWindow),
      logModel(new RequestLogModel(this)),
      proxyModel(new RequestLogFilterProxy(this)),
      adRepository_(adRepository)
{
    ui->setupUi(this);

    proxyModel->setSourceModel(logModel);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(-1);

    ui->tableViewLogs->setModel(proxyModel);
    ui->tableViewLogs->horizontalHeader()->setStretchLastSection(true);
    ui->tableViewLogs->verticalHeader()->setVisible(false);

    populateCommandFilter();
    populateStatusFilter();
    setupConnections();
    applyFilters();

    uptimeTimer.setInterval(1000);
    connect(&uptimeTimer, &QTimer::timeout, this, &ServerConsoleWindow::updateUptime);
}

ServerConsoleWindow::~ServerConsoleWindow() {
    delete ui;
}

void ServerConsoleWindow::setupConnections() {
    connect(ui->lineEditFilterText, &QLineEdit::textChanged,
            this, &ServerConsoleWindow::applyFilters);
    connect(ui->comboBoxCommandFilter, &QComboBox::currentTextChanged,
            this, &ServerConsoleWindow::applyFilters);
    connect(ui->comboBoxStatusFilter, &QComboBox::currentTextChanged,
            this, &ServerConsoleWindow::applyFilters);

    connect(ui->pushButtonClearFilters, &QPushButton::clicked,
            this, &ServerConsoleWindow::clearFilters);
    connect(ui->pushButtonClearLog, &QPushButton::clicked,
            this, &ServerConsoleWindow::clearLog);
    connect(ui->pushButtonExport, &QPushButton::clicked,
            this, &ServerConsoleWindow::exportLogs);
    connect(ui->pushButtonPause, &QPushButton::toggled,
            this, &ServerConsoleWindow::togglePause);
    connect(ui->pushButtonQuit, &QPushButton::clicked,
            this, &QWidget::close);
    connect(ui->pushButtonShowAdQueue, &QPushButton::clicked,
            this, &ServerConsoleWindow::showPendingAdsWindow);
}

void ServerConsoleWindow::populateCommandFilter() {
    ui->comboBoxCommandFilter->clear();

    QStringList commands = {
        tr("All Commands"),
        tr("Ping"),
        tr("Pong"),
        tr("Signup"),
        tr("Signup Result"),
        tr("Login"),
        tr("Login Result"),
        tr("Logout"),
        tr("Logout Result"),
        tr("Session Refresh"),
        tr("Session Refresh Result"),
        tr("Profile Update"),
        tr("Profile Update Result"),
        tr("Profile History"),
        tr("Profile History Result"),
        tr("Admin Stats"),
        tr("Admin Stats Result"),
        tr("Captcha Challenge"),
        tr("Captcha Challenge Result"),
        tr("Ad Create"),
        tr("Ad Create Result"),
        tr("Ad Update"),
        tr("Ad Update Result"),
        tr("Ad Delete"),
        tr("Ad Delete Result"),
        tr("Ad List"),
        tr("Ad List Result"),
        tr("Ad Detail"),
        tr("Ad Detail Result"),
        tr("Ad Status Update"),
        tr("Ad Status Notify"),
        tr("Category List"),
        tr("Category List Result"),
        tr("Cart Add Item"),
        tr("Cart Add Item Result"),
        tr("Cart Remove Item"),
        tr("Cart Remove Item Result"),
        tr("Cart List"),
        tr("Cart List Result"),
        tr("Cart Clear"),
        tr("Cart Clear Result"),
        tr("Buy"),
        tr("Buy Result"),
        tr("Transaction History"),
        tr("Transaction History Result"),
        tr("Wallet Balance"),
        tr("Wallet Balance Result"),
        tr("Wallet TopUp"),
        tr("Wallet TopUp Result"),
        tr("Wallet Adjust Notify"),
        tr("System Notification"),
        tr("Error")
    };
    ui->comboBoxCommandFilter->addItems(commands);
}

void ServerConsoleWindow::populateStatusFilter() {
    ui->comboBoxStatusFilter->clear();

    ui->comboBoxStatusFilter->addItems({
        tr("All Statuses"),
        QStringLiteral("200"),
        QStringLiteral("400"),
        QStringLiteral("401"),
        QStringLiteral("403"),
        QStringLiteral("404"),
        QStringLiteral("409"),
        QStringLiteral("402"),
        QStringLiteral("410"),
        QStringLiteral("503"),
        QStringLiteral("500")
    });
}

void ServerConsoleWindow::applyFilters()
{
    const QString text = ui->lineEditFilterText->text();
    const QString command = ui->comboBoxCommandFilter->currentText();
    const QString status = ui->comboBoxStatusFilter->currentText();

    proxyModel->setCommandFilter(command);
    proxyModel->setStatusFilter(status);
    proxyModel->setTextFilter(text);
}


void ServerConsoleWindow::clearFilters() {
    ui->lineEditFilterText->clear();
    ui->comboBoxCommandFilter->setCurrentIndex(0);
    ui->comboBoxStatusFilter->setCurrentIndex(0);
    applyFilters();
}

void ServerConsoleWindow::clearLog() {
    if (QMessageBox::question(this, tr("Clear Log"),
                              tr("Are you sure you want to remove all log entries?")) == QMessageBox::Yes) {
        logModel->clear();
    }
}

void ServerConsoleWindow::exportLogs() {
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Export Logs"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/server-log.csv",
        tr("CSV files (*.csv);;All files (*)")
    );
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Export Logs"), tr("Could not write to %1").arg(filePath));
        return;
    }

    QTextStream stream(&file);
    stream << "timestamp,remote,user,command,status,error,message\n";

    for (int row = 0; row < proxyModel->rowCount(); ++row) {
        const auto timestamp = proxyModel->index(row, 0).data().toString();
        const auto remote    = proxyModel->index(row, 1).data().toString();
        const auto user      = proxyModel->index(row, 2).data().toString();
        const auto command   = proxyModel->index(row, 3).data().toString();
        const auto status    = proxyModel->index(row, 4).data().toString();
        const auto error     = proxyModel->index(row, 5).data().toString();
        const auto message   = proxyModel->index(row, 6).data().toString();

        stream << QString("%1,%2,%3,%4,%5,%6,%7\n")
                      .arg(timestamp, remote, user, command, status, error, message);
    }
}

void ServerConsoleWindow::togglePause(bool pause) {
    paused = pause;
    ui->pushButtonPause->setText(pause ? tr("Resume Stream") : tr("Pause Stream"));
    ui->statusbar->showMessage(pause ? tr("Log streaming paused") : tr("Log streaming resumed"), 3000);
}


void ServerConsoleWindow::showPendingAdsWindow()
{
    if (pendingAdsWindow == nullptr) {
        pendingAdsWindow = new PendingAdsWindow(adRepository_, this);
    }

    pendingAdsWindow->show();
    pendingAdsWindow->raise();
    pendingAdsWindow->activateWindow();
}

void ServerConsoleWindow::updateUptime() {
    if (!serverStartTime.isValid()) {
        ui->labelUptimeValue->setText("00:00:00");
        return;
    }
    const qint64 secs = serverStartTime.secsTo(QDateTime::currentDateTime());
    const QTime time = QTime(0,0).addSecs(static_cast<int>(secs));
    ui->labelUptimeValue->setText(time.toString("hh:mm:ss"));
}

void ServerConsoleWindow::onServerStarted(quint16 /*port*/)
{
    ui->labelStatusValue->setText(tr("Online"));
    ui->labelStatusValue->setStyleSheet(QStringLiteral("color: rgb(70, 180, 70); font-weight: bold;"));
    serverStartTime = QDateTime::currentDateTime();
    uptimeTimer.start();
}


void ServerConsoleWindow::onServerStopped() {
    ui->labelStatusValue->setText(tr("Offline"));
    ui->labelStatusValue->setStyleSheet("color: rgb(200, 50, 50); font-weight: bold;");
    ui->labelConnectionCountValue->setText("0");
    uptimeTimer.stop();
}

void ServerConsoleWindow::onActiveConnectionCountChanged(int count) {
    ui->labelConnectionCountValue->setNum(count);
}

void ServerConsoleWindow::onRequestLogged(const RequestLogEntry& entry) {
    if (paused) {
        return;
    }
    logModel->appendLog(entry);
}


void ServerConsoleWindow::onRequestProcessed(const common::Message& request,
                                             const common::Message& response)
{
    RequestLogEntry entry;

    entry.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    entry.remoteAddress = QStringLiteral("-"); // در صورت نیاز، بعداً از ClientConnection پاس داده شود

    if (!response.sessionToken().isEmpty()) {
        entry.sessionToken = response.sessionToken();
    } else {
        entry.sessionToken = request.sessionToken();
    }

    entry.username = extractUsername(request.payload());
    const common::Command commandForLog =
        (response.command() == common::Command::Error)
            ? common::Command::Error
            : request.command();
    entry.command = mapCommandToText(commandForLog);

    if (response.status() == common::MessageStatus::Success) {
        entry.statusCode = 200;
    } else if (response.status() == common::MessageStatus::Failure) {
        entry.statusCode = common::errorCodeToStatusCode(response.errorCode());
    } else {
        entry.statusCode = 0;
    }

    entry.errorCode = mapErrorCodeToText(response.errorCode());
    entry.message = response.statusMessage();

    if (entry.message.isEmpty()) {
        const QJsonObject responsePayload = response.payload();
        entry.message = responsePayload.value(QStringLiteral("message")).toString();
        if (entry.message.isEmpty()) {
            entry.message = responsePayload.value(QStringLiteral("reason")).toString();
        }
    }

    if (entry.message.isEmpty()) {
        entry.message = QStringLiteral("-");
    }

    onRequestLogged(entry);
}

QString ServerConsoleWindow::mapCommandToText(common::Command command) const
{
    switch (command) {
    case common::Command::Unknown:              return tr("Unknown");
    case common::Command::Ping:                 return tr("Ping");
    case common::Command::Pong:                 return tr("Pong");
    case common::Command::Error:                return tr("Error");
    case common::Command::Login:                return tr("Login");
    case common::Command::LoginResult:          return tr("Login Result");
    case common::Command::Signup:               return tr("Signup");
    case common::Command::SignupResult:         return tr("Signup Result");
    case common::Command::Logout:               return tr("Logout");
    case common::Command::LogoutResult:         return tr("Logout Result");
    case common::Command::SessionRefresh:       return tr("Session Refresh");
    case common::Command::SessionRefreshResult: return tr("Session Refresh Result");
    case common::Command::ProfileUpdate:        return tr("Profile Update");
    case common::Command::ProfileUpdateResult:  return tr("Profile Update Result");
    case common::Command::ProfileHistory:       return tr("Profile History");
    case common::Command::ProfileHistoryResult: return tr("Profile History Result");
    case common::Command::AdminStats:           return tr("Admin Stats");
    case common::Command::AdminStatsResult:     return tr("Admin Stats Result");
    case common::Command::CaptchaChallenge:     return tr("Captcha Challenge");
    case common::Command::CaptchaChallengeResult:return tr("Captcha Challenge Result");
    case common::Command::AdCreate:             return tr("Ad Create");
    case common::Command::AdCreateResult:       return tr("Ad Create Result");
    case common::Command::AdUpdate:             return tr("Ad Update");
    case common::Command::AdUpdateResult:       return tr("Ad Update Result");
    case common::Command::AdDelete:             return tr("Ad Delete");
    case common::Command::AdDeleteResult:       return tr("Ad Delete Result");
    case common::Command::AdList:               return tr("Ad List");
    case common::Command::AdListResult:         return tr("Ad List Result");
    case common::Command::AdDetail:             return tr("Ad Detail");
    case common::Command::AdDetailResult:       return tr("Ad Detail Result");
    case common::Command::AdStatusUpdate:       return tr("Ad Status Update");
    case common::Command::AdStatusNotify:       return tr("Ad Status Notify");
    case common::Command::CategoryList:         return tr("Category List");
    case common::Command::CategoryListResult:   return tr("Category List Result");
    case common::Command::CartAddItem:          return tr("Cart Add Item");
    case common::Command::CartAddItemResult:    return tr("Cart Add Item Result");
    case common::Command::CartRemoveItem:       return tr("Cart Remove Item");
    case common::Command::CartRemoveItemResult: return tr("Cart Remove Item Result");
    case common::Command::CartList:             return tr("Cart List");
    case common::Command::CartListResult:       return tr("Cart List Result");
    case common::Command::CartClear:            return tr("Cart Clear");
    case common::Command::CartClearResult:      return tr("Cart Clear Result");
    case common::Command::Buy:                  return tr("Buy");
    case common::Command::BuyResult:            return tr("Buy Result");
    case common::Command::TransactionHistory:   return tr("Transaction History");
    case common::Command::TransactionHistoryResult:return tr("Transaction History Result");
    case common::Command::WalletBalance:        return tr("Wallet Balance");
    case common::Command::WalletBalanceResult:  return tr("Wallet Balance Result");
    case common::Command::WalletTopUp:          return tr("Wallet TopUp");
    case common::Command::WalletTopUpResult:    return tr("Wallet TopUp Result");
    case common::Command::WalletAdjustNotify:   return tr("Wallet Adjust Notify");
    case common::Command::SystemNotification:   return tr("System Notification");
    }
    return tr("Unknown (%1)").arg(static_cast<int>(command));
}

QString ServerConsoleWindow::mapErrorCodeToText(common::ErrorCode code) const
{
    switch (code) {
    case common::ErrorCode::None:                 return tr("None");
    case common::ErrorCode::UnknownCommand:       return tr("Unknown Command");
    case common::ErrorCode::InvalidJson:          return tr("Invalid JSON");
    case common::ErrorCode::InvalidPayload:       return tr("Invalid Payload");
    case common::ErrorCode::AuthInvalidCredentials:return tr("Invalid Credentials");
    case common::ErrorCode::AuthSessionExpired:   return tr("Session Expired");
    case common::ErrorCode::AuthUnauthorized:     return tr("Unauthorized");
    case common::ErrorCode::ValidationFailed:     return tr("Validation Failed");
    case common::ErrorCode::NotFound:             return tr("Not Found");
    case common::ErrorCode::AlreadyExists:        return tr("Already Exists");
    case common::ErrorCode::PermissionDenied:     return tr("Permission Denied");
    case common::ErrorCode::InsufficientFunds:    return tr("Insufficient Funds");
    case common::ErrorCode::AdNotAvailable:       return tr("Ad Not Available");
    case common::ErrorCode::DuplicateAd:          return tr("Duplicate Ad");
    case common::ErrorCode::DatabaseError:        return tr("Database Error");
    case common::ErrorCode::InternalError:        return tr("Internal Error");
    default:
        return tr("Unknown (%1)").arg(static_cast<int>(code));
    }
}


QString ServerConsoleWindow::extractUsername(const QJsonObject& payload) const
{
    const auto usernameValue = payload.value(QStringLiteral("username"));
    if (usernameValue.isString()) {
        return usernameValue.toString();
    }

    const auto userObject = payload.value(QStringLiteral("user"));
    if (userObject.isObject()) {
        const auto nestedUsername = userObject.toObject().value(QStringLiteral("username"));
        if (nestedUsername.isString()) {
            return nestedUsername.toString();
        }
    }

    return QStringLiteral("-");
}
