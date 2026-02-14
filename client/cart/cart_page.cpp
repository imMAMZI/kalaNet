#include "cart_page.h"
#include "ui_cart_page.h"

#include <QHeaderView>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QMessageBox>

cart_page::cart_page(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::cart_page)
{
    ui->setupUi(this);

    setupCartTable();

    walletBalanceTokens = 200;
    setWalletBalanceLabel(walletBalanceTokens);

    loadDummyCart();
    refreshCartTable();
    recomputeTotals();
}

cart_page::~cart_page()
{
    delete ui;
}

void cart_page::setItems(const QVector<CartItemData> &newItems)
{
    items = newItems;
    refreshCartTable();
    recomputeTotals();
}

QVector<cart_page::CartItemData> cart_page::itemsData() const
{
    return items;
}

void cart_page::setupCartTable()
{
    ui->twCart->setColumnCount(6);

    ui->twCart->horizontalHeader()->setStretchLastSection(true);
    ui->twCart->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->twCart->verticalHeader()->setVisible(false);

    ui->twCart->setColumnWidth(0, 240);
    ui->twCart->setColumnWidth(1, 140);
    ui->twCart->setColumnWidth(2, 100);
    ui->twCart->setColumnWidth(3, 140);
    ui->twCart->setColumnWidth(4, 110);
    ui->twCart->setColumnWidth(5, 140);

    ui->twCart->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->twCart->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->twCart->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void cart_page::loadDummyCart()
{
    items.clear();
    items.push_back({"Headphones", "Electronics", 55, "Mehdi", "Available"});
    items.push_back({"C++ Book", "Books", 18, "Neda", "Available"});
    items.push_back({"Wood Chair", "Home", 35, "Sara", "Available"});
}

void cart_page::refreshCartTable()
{
    ui->twCart->setRowCount(static_cast<int>(items.size()));

    for (int row = 0; row < static_cast<int>(items.size()); ++row) {
        const CartItemData& it = items[row];

        ui->twCart->setItem(row, 0, new QTableWidgetItem(it.title));
        ui->twCart->setItem(row, 1, new QTableWidgetItem(it.category));
        ui->twCart->setItem(row, 2, new QTableWidgetItem(QString::number(it.priceTokens) + " token"));
        ui->twCart->setItem(row, 3, new QTableWidgetItem(it.seller));
        ui->twCart->setItem(row, 4, new QTableWidgetItem(it.status));

        auto *btnRemove = new QPushButton("Remove");
        btnRemove->setCursor(Qt::PointingHandCursor);
        btnRemove->setMinimumHeight(34);
        btnRemove->setStyleSheet(
            "QPushButton{background:transparent;color:#FF5C7A;border:1px solid #FF5C7A;border-radius:12px;padding:6px 10px;font-weight:900;}"
            "QPushButton:hover{background:rgba(255,92,122,0.12);}"
            "QPushButton:pressed{background:rgba(255,92,122,0.20);}"
        );

        connect(btnRemove, &QPushButton::clicked, this, [this, row]() {
            removeItemAt(row);
        });

        ui->twCart->setCellWidget(row, 5, btnRemove);
    }
}

void cart_page::removeItemAt(int row)
{
    if (row < 0 || row >= static_cast<int>(items.size())) return;

    items.removeAt(row);
    refreshCartTable();
    recomputeTotals();
    notifyItemsChanged();
}

void cart_page::clearAll()
{
    items.clear();
    refreshCartTable();
    recomputeTotals();
    notifyItemsChanged();
}

void cart_page::notifyItemsChanged()
{
    emit cartItemsChanged(items);
}

int cart_page::computeDiscountTokens(const QString& code, int subtotal) const
{
    if (code.compare("OFF10", Qt::CaseInsensitive) == 0) {
        int d = (subtotal * 10) / 100;
        return (d > 50) ? 50 : d;
    }
    if (code.compare("OFF20", Qt::CaseInsensitive) == 0) {
        int d = (subtotal * 20) / 100;
        return (d > 100) ? 100 : d;
    }
    return 0;
}

void cart_page::recomputeTotals()
{
    subtotalTokens = 0;
    for (const auto& it : items) subtotalTokens += it.priceTokens;

    const QString code = ui->leDiscountCode->text().trimmed();
    discountTokens = computeDiscountTokens(code, subtotalTokens);

    totalTokens = subtotalTokens - discountTokens;
    if (totalTokens < 0) totalTokens = 0;

    setTotalsLabels(subtotalTokens, discountTokens, totalTokens);

    emit cartChanged(static_cast<int>(items.size()), subtotalTokens, discountTokens, totalTokens);
}

void cart_page::setWalletBalanceLabel(int tokens)
{
    ui->lblWalletBalance->setText(QString::number(tokens) + " token");
}

void cart_page::setTotalsLabels(int subtotal, int discount, int total)
{
    ui->lblSubtotalValue->setText(QString::number(subtotal) + " token");
    ui->lblDiscountValue->setText(QString::number(discount) + " token");
    ui->lblTotalValue->setText(QString::number(total) + " token");
}

void cart_page::on_btnBackToMenu_clicked()
{
    emit backToMenuRequested();
}

void cart_page::on_btnClearAll_clicked()
{
    clearAll();
}

void cart_page::on_btnApplyDiscount_clicked()
{
    recomputeTotals();

    const QString code = ui->leDiscountCode->text().trimmed();
    if (code.isEmpty()) {
        ui->lblDiscountStatus->setText("Enter a discount code to apply.");
    } else if (discountTokens > 0) {
        ui->lblDiscountStatus->setText("Discount applied.");
    } else {
        ui->lblDiscountStatus->setText("Invalid or unsupported code (for now).");
    }
}

void cart_page::on_btnPurchase_clicked()
{
    if (items.isEmpty()) {
        QMessageBox::information(this, "Purchase", "Your cart is empty.");
        return;
    }

    recomputeTotals();

    if (walletBalanceTokens < totalTokens) {
        QMessageBox::warning(this, "Purchase failed", "Not enough tokens in wallet.");
        return;
    }
    walletBalanceTokens -= totalTokens;
    setWalletBalanceLabel(walletBalanceTokens);

    const QVector<CartItemData> purchasedItems = items;
    const QString discountCode = ui->leDiscountCode->text().trimmed();

    QMessageBox::information(this, "Purchase", "Purchase successful (stub).");

    emit purchaseCompleted(purchasedItems, discountCode);
    emit purchaseRequested(discountCode);

    clearAll();
    ui->leDiscountCode->clear();
    ui->lblDiscountStatus->clear();
}
