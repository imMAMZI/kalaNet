#include "discount_code_manager_window.h"

#include "../repository/wallet_repository.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

DiscountCodeManagerWindow::DiscountCodeManagerWindow(WalletRepository& walletRepository, QWidget* parent)
    : QMainWindow(parent),
      walletRepository_(walletRepository)
{
    setWindowTitle(QStringLiteral("Discount Code Manager"));
    resize(920, 560);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    table_ = new QTableWidget(0, 8, this);
    table_->setHorizontalHeaderLabels({QStringLiteral("Code"), QStringLiteral("Type"), QStringLiteral("Value"),
                                       QStringLiteral("Max Discount"), QStringLiteral("Min Subtotal"),
                                       QStringLiteral("Usage"), QStringLiteral("Active"), QStringLiteral("Expires At")});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->verticalHeader()->setVisible(false);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto* formLayout = new QFormLayout();
    codeEdit_ = new QLineEdit(this);
    typeCombo_ = new QComboBox(this);
    typeCombo_->addItems({QStringLiteral("percent"), QStringLiteral("fixed")});
    valueSpin_ = new QSpinBox(this);
    valueSpin_->setRange(1, 100000);
    maxDiscountSpin_ = new QSpinBox(this);
    maxDiscountSpin_->setRange(0, 100000);
    minSubtotalSpin_ = new QSpinBox(this);
    minSubtotalSpin_->setRange(0, 100000);
    usageLimitSpin_ = new QSpinBox(this);
    usageLimitSpin_->setRange(1, 1000000);
    unlimitedUsageCheck_ = new QCheckBox(QStringLiteral("Unlimited usage"), this);
    unlimitedUsageCheck_->setChecked(true);
    activeCheck_ = new QCheckBox(QStringLiteral("Active"), this);
    activeCheck_->setChecked(true);
    expiresCheck_ = new QCheckBox(QStringLiteral("Has expiry"), this);
    expiresEdit_ = new QDateTimeEdit(QDateTime::currentDateTimeUtc().addDays(30), this);
    expiresEdit_->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm"));
    expiresEdit_->setCalendarPopup(true);
    expiresEdit_->setEnabled(false);

    formLayout->addRow(QStringLiteral("Code"), codeEdit_);
    formLayout->addRow(QStringLiteral("Type"), typeCombo_);
    formLayout->addRow(QStringLiteral("Value"), valueSpin_);
    formLayout->addRow(QStringLiteral("Max Discount"), maxDiscountSpin_);
    formLayout->addRow(QStringLiteral("Min Subtotal"), minSubtotalSpin_);
    formLayout->addRow(QStringLiteral("Usage Limit"), usageLimitSpin_);
    formLayout->addRow(QString(), unlimitedUsageCheck_);
    formLayout->addRow(QString(), activeCheck_);
    formLayout->addRow(QString(), expiresCheck_);
    formLayout->addRow(QStringLiteral("Expires At"), expiresEdit_);

    auto* buttonRow = new QHBoxLayout();
    auto* refreshButton = new QPushButton(QStringLiteral("Refresh"), this);
    auto* clearButton = new QPushButton(QStringLiteral("New"), this);
    auto* saveButton = new QPushButton(QStringLiteral("Save"), this);
    deleteButton_ = new QPushButton(QStringLiteral("Delete"), this);
    deleteButton_->setEnabled(false);

    buttonRow->addWidget(refreshButton);
    buttonRow->addWidget(clearButton);
    buttonRow->addStretch();
    buttonRow->addWidget(saveButton);
    buttonRow->addWidget(deleteButton_);

    layout->addWidget(table_);
    layout->addLayout(formLayout);
    layout->addLayout(buttonRow);
    setCentralWidget(central);

    connect(refreshButton, &QPushButton::clicked, this, &DiscountCodeManagerWindow::loadCodes);
    connect(clearButton, &QPushButton::clicked, this, &DiscountCodeManagerWindow::clearForm);
    connect(saveButton, &QPushButton::clicked, this, &DiscountCodeManagerWindow::saveCode);
    connect(deleteButton_, &QPushButton::clicked, this, &DiscountCodeManagerWindow::deleteSelectedCode);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &DiscountCodeManagerWindow::onRowSelected);
    connect(unlimitedUsageCheck_, &QCheckBox::toggled, usageLimitSpin_, &QWidget::setDisabled);
    connect(expiresCheck_, &QCheckBox::toggled, expiresEdit_, &QWidget::setEnabled);

    loadCodes();
}

void DiscountCodeManagerWindow::loadCodes()
{
    table_->setRowCount(0);

    try {
        const auto rows = walletRepository_.listDiscountCodes();
        for (const auto& row : rows) {
            const int r = table_->rowCount();
            table_->insertRow(r);
            table_->setItem(r, 0, new QTableWidgetItem(row.code));
            table_->setItem(r, 1, new QTableWidgetItem(row.type));
            table_->setItem(r, 2, new QTableWidgetItem(QString::number(row.valueTokens)));
            table_->setItem(r, 3, new QTableWidgetItem(QString::number(row.maxDiscountTokens)));
            table_->setItem(r, 4, new QTableWidgetItem(QString::number(row.minSubtotalTokens)));
            const QString usage = row.usageLimit < 0
                                      ? QStringLiteral("%1 / ∞").arg(row.usedCount)
                                      : QStringLiteral("%1 / %2").arg(row.usedCount).arg(row.usageLimit);
            table_->setItem(r, 5, new QTableWidgetItem(usage));
            table_->setItem(r, 6, new QTableWidgetItem(row.active ? QStringLiteral("Yes") : QStringLiteral("No")));
            table_->setItem(r, 7, new QTableWidgetItem(row.expiresAt.isValid() ? row.expiresAt.toString(Qt::ISODate) : QStringLiteral("Never")));
        }
    } catch (const std::exception& ex) {
        QMessageBox::warning(this, QStringLiteral("Discount Codes"),
                             QStringLiteral("Failed to load discount codes: %1").arg(ex.what()));
    }
}

void DiscountCodeManagerWindow::saveCode()
{
    WalletRepository::DiscountValidationResult record;
    record.code = codeEdit_->text().trimmed().toUpper();
    record.type = typeCombo_->currentText();
    record.valueTokens = valueSpin_->value();
    record.maxDiscountTokens = maxDiscountSpin_->value();
    record.minSubtotalTokens = minSubtotalSpin_->value();
    record.usageLimit = unlimitedUsageCheck_->isChecked() ? -1 : usageLimitSpin_->value();
    record.active = activeCheck_->isChecked();
    record.expiresAt = expiresCheck_->isChecked() ? expiresEdit_->dateTime().toUTC() : QDateTime();

    QString error;
    if (!walletRepository_.upsertDiscountCode(record, &error)) {
        QMessageBox::warning(this, QStringLiteral("Discount Codes"),
                             error.isEmpty() ? QStringLiteral("Failed to save code") : error);
        return;
    }

    QMessageBox::information(this, QStringLiteral("Discount Codes"), QStringLiteral("Discount code saved."));
    loadCodes();
}

void DiscountCodeManagerWindow::deleteSelectedCode()
{
    const auto selected = table_->selectedItems();
    if (selected.isEmpty()) {
        return;
    }
    const QString code = table_->item(selected.first()->row(), 0)->text();

    QString error;
    if (!walletRepository_.deleteDiscountCode(code, &error)) {
        QMessageBox::warning(this, QStringLiteral("Discount Codes"),
                             error.isEmpty() ? QStringLiteral("Failed to delete code") : error);
        return;
    }

    clearForm();
    loadCodes();
}

void DiscountCodeManagerWindow::onRowSelected()
{
    const auto selected = table_->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    const int row = selected.first()->row();
    codeEdit_->setText(table_->item(row, 0)->text());
    typeCombo_->setCurrentText(table_->item(row, 1)->text());
    valueSpin_->setValue(table_->item(row, 2)->text().toInt());
    maxDiscountSpin_->setValue(table_->item(row, 3)->text().toInt());
    minSubtotalSpin_->setValue(table_->item(row, 4)->text().toInt());

    const QString usageText = table_->item(row, 5)->text();
    if (usageText.contains('/')) {
        const QString limitStr = usageText.section('/', 1, 1).trimmed();
        const bool unlimited = limitStr == QStringLiteral("∞");
        unlimitedUsageCheck_->setChecked(unlimited);
        if (!unlimited) {
            usageLimitSpin_->setValue(limitStr.toInt());
        }
    }

    activeCheck_->setChecked(table_->item(row, 6)->text() == QStringLiteral("Yes"));

    const QString exp = table_->item(row, 7)->text();
    const QDateTime expiry = QDateTime::fromString(exp, Qt::ISODate);
    const bool hasExpiry = expiry.isValid();
    expiresCheck_->setChecked(hasExpiry);
    if (hasExpiry) {
        expiresEdit_->setDateTime(expiry);
    }

    deleteButton_->setEnabled(true);
}

void DiscountCodeManagerWindow::clearForm()
{
    codeEdit_->clear();
    typeCombo_->setCurrentIndex(0);
    valueSpin_->setValue(10);
    maxDiscountSpin_->setValue(0);
    minSubtotalSpin_->setValue(0);
    usageLimitSpin_->setValue(100);
    unlimitedUsageCheck_->setChecked(true);
    activeCheck_->setChecked(true);
    expiresCheck_->setChecked(false);
    expiresEdit_->setDateTime(QDateTime::currentDateTimeUtc().addDays(30));
    deleteButton_->setEnabled(false);
}
