// server/ui/serverconsolewindow.cpp
#include <QApplication>

#include "server_console_window.h"
#include "ui_server_console_window.h"
#include "request_log_model.h"
#include "request_log_filter_proxy.h"
#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QTextStream>

ServerConsoleWindow::ServerConsoleWindow(QWidget* parent)
    : QMainWindow(parent),
      ui(new Ui::ServerConsoleWindow),
      logModel(new RequestLogModel(this)),
      proxyModel(new RequestLogFilterProxy(this))
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
}

void ServerConsoleWindow::populateCommandFilter() {
    ui->comboBoxCommandFilter->clear();

    QStringList commands = {
        tr("All Commands"),
        tr("Signup"),
        tr("Signup Result"),
        tr("Login"),
        tr("Login Result"),
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
    common::Command commandForLog = response.command();
    if (commandForLog == common::Command::Unknown) {
        commandForLog = request.command();
    }
    entry.command = mapCommandToText(commandForLog);
    entry.statusCode = static_cast<int>(response.status());

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
    case common::Command::Signup:          return tr("Signup");
    case common::Command::SignupResult:    return tr("Signup Result");
    case common::Command::Login:           return tr("Login");
    case common::Command::LoginResult:     return tr("Login Result");
    case common::Command::Error:           return tr("Error");
        // سایر فرمان‌ها را طبق نیاز پروژه اضافه کنید
    default:
        return tr("Unknown (%1)").arg(static_cast<int>(command));
    }
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
