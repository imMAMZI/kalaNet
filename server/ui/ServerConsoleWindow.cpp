// server/ui/serverconsolewindow.cpp
#include <QApplication>

#include "serverconsolewindow.h"
#include "ui_ServerConsoleWindow.h"

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
      proxyModel(new QSortFilterProxyModel(this))
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
    // بعداً لیست واقعی فرمان‌ها از common/commands تامین می‌شود.
    QStringList commands = {
        tr("All Commands"),
        QStringLiteral("AUTH_LOGIN"),
        QStringLiteral("AUTH_SIGNUP"),
        QStringLiteral("AD_CREATE"),
        QStringLiteral("AD_APPROVE"),
        QStringLiteral("AD_REJECT"),
        QStringLiteral("TRANSACTION_CHECKOUT")
    };
    ui->comboBoxCommandFilter->addItems(commands);
}

void ServerConsoleWindow::populateStatusFilter() {
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

void ServerConsoleWindow::applyFilters() {
    const auto text = ui->lineEditFilterText->text();
    proxyModel->setFilterFixedString(text);

    const QString command = ui->comboBoxCommandFilter->currentText();
    const QString status = ui->comboBoxStatusFilter->currentText();

    proxyModel->setFilterRole(Qt::UserRole);
    proxyModel->setFilterRegularExpression({});
    proxyModel->setFilterWildcard({});

    proxyModel->setFilterAcceptsRow([=](int sourceRow, const QModelIndex& sourceParent) {
        const auto indexCommand = logModel->index(sourceRow, 3, sourceParent);
        const auto indexStatus = logModel->index(sourceRow, 4, sourceParent);
        const auto indexError  = logModel->index(sourceRow, 5, sourceParent);
        const auto indexMessage= logModel->index(sourceRow, 6, sourceParent);
        const auto indexUser   = logModel->index(sourceRow, 2, sourceParent);
        const auto indexRemote = logModel->index(sourceRow, 1, sourceParent);

        const auto commandValue = logModel->data(indexCommand).toString();
        const auto statusValue  = logModel->data(indexStatus).toString();
        const auto errorValue   = logModel->data(indexError).toString();
        const auto messageValue = logModel->data(indexMessage).toString();
        const auto userValue    = logModel->data(indexUser).toString();
        const auto remoteValue  = logModel->data(indexRemote).toString();

        if (command != tr("All Commands") && commandValue != command) {
            return false;
        }
        if (status != tr("All Statuses") && statusValue != status) {
            return false;
        }
        if (!text.isEmpty()) {
            const auto haystack = QStringList{
                commandValue, statusValue, errorValue, messageValue, userValue, remoteValue
            }.join(' ').toLower();
            if (!haystack.contains(text.toLower())) {
                return false;
            }
        }
        return true;
    });

    // این خط برای مجبور کردن QSortFilterProxyModel به فیلتر مجدد است.
    proxyModel->invalidate();
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

void ServerConsoleWindow::onServerStarted() {
    ui->labelStatusValue->setText(tr("Online"));
    ui->labelStatusValue->setStyleSheet("color: rgb(70, 180, 70); font-weight: bold;");
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
