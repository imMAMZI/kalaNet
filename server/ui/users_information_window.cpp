#include "users_information_window.h"

#include "../auth/session_service.h"
#include "../repository/user_repository.h"

#include <QHeaderView>
#include <QBrush>
#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include <exception>

UsersInformationWindow::UsersInformationWindow(UserRepository& userRepository,
                                               SessionService& sessionService,
                                               QWidget* parent)
    : QMainWindow(parent),
      userRepository_(userRepository),
      sessionService_(sessionService)
{
    setWindowTitle(QStringLiteral("Users Information"));
    resize(1050, 620);

    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget { background-color: #1f232b; color: #e7ebf3; }"
        "QLabel { color: #f1f4fb; }"
        "QTableWidget { background-color: #252b36; color: #e7ebf3; border: 1px solid #465064; border-radius: 6px; selection-background-color: #3f6eb8; selection-color: #ffffff; }"
        "QHeaderView::section { background-color: #333b49; color: #eaf0fa; padding: 5px; border: none; border-right: 1px solid #4b5568; }"
        "QPushButton { border: 1px solid #5b6680; border-radius: 6px; padding: 6px 10px; background-color: #353d4c; color: #f1f4fb; }"
        "QPushButton:hover { background-color: #414c61; }"
        "QLineEdit { border: 1px solid #5b6680; border-radius: 6px; padding: 6px 8px; background-color: #202632; color: #eef3ff; }"));

    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);

    auto* topBarLayout = new QHBoxLayout();
    auto* searchLabel = new QLabel(QStringLiteral("Search Users:"), this);
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText(QStringLiteral("Search by name or username..."));
    auto* refreshButton = new QPushButton(QStringLiteral("Refresh"), this);

    topBarLayout->addWidget(searchLabel);
    topBarLayout->addWidget(searchEdit_, 1);
    topBarLayout->addWidget(refreshButton);

    table_ = new QTableWidget(0, 8, this);
    table_->setHorizontalHeaderLabels({QStringLiteral("Name"),
                                       QStringLiteral("Username"),
                                       QStringLiteral("Mobile Number"),
                                       QStringLiteral("Password"),
                                       QStringLiteral("Status"),
                                       QStringLiteral("Sold Items"),
                                       QStringLiteral("Bought Items"),
                                       QStringLiteral("Role")});
    table_->verticalHeader()->setVisible(false);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->horizontalHeader()->setStretchLastSection(true);

    mainLayout->addLayout(topBarLayout);
    mainLayout->addWidget(table_);

    setCentralWidget(central);

    connect(searchEdit_, &QLineEdit::textChanged, this, &UsersInformationWindow::loadUsers);
    connect(refreshButton, &QPushButton::clicked, this, &UsersInformationWindow::loadUsers);

    loadUsers();
}

void UsersInformationWindow::loadUsers()
{
    table_->setRowCount(0);

    try {
        const auto users = userRepository_.listUsersForAdmin(searchEdit_->text());
        const auto activeUsers = sessionService_.activeUsernames();

        for (const auto& user : users) {
            const int row = table_->rowCount();
            table_->insertRow(row);
            table_->setItem(row, 0, new QTableWidgetItem(user.fullName));
            table_->setItem(row, 1, new QTableWidgetItem(user.username));
            table_->setItem(row, 2, new QTableWidgetItem(user.phone));
            table_->setItem(row, 3, new QTableWidgetItem(user.passwordHash));

            const bool isOnline = activeUsers.contains(user.username.trimmed());
            auto* statusItem = new QTableWidgetItem(isOnline ? QStringLiteral("Online") : QStringLiteral("Offline"));
            statusItem->setForeground(isOnline ? QBrush(QColor("#73da85")) : QBrush(QColor("#f08d8d")));
            table_->setItem(row, 4, statusItem);

            table_->setItem(row, 5, new QTableWidgetItem(QString::number(user.soldItems)));
            table_->setItem(row, 6, new QTableWidgetItem(QString::number(user.boughtItems)));
            table_->setItem(row, 7, new QTableWidgetItem(user.role));
        }

        table_->resizeColumnsToContents();
    } catch (const std::exception& ex) {
        QMessageBox::warning(this,
                             QStringLiteral("Users Information"),
                             QStringLiteral("Failed to load users information: %1").arg(ex.what()));
    }
}
