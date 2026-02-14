#include "cart_page.h"
#include "ui_cart_page.h"

#include "../network/auth_client.h"

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
    setWalletBalanceLabel(0);

    connect(AuthClient::instance(), &AuthClient::cartListReceived, this,
            [this](bool success, const QString& message, const QJsonArray& data) {
                if (!success) {
                    QMessageBox::warning(this, "Cart", message);
                    return;
                }

                QVector<CartItemData> serverItems;
                for (const QJsonValue& value : data) {
                    const QJsonObject item = value.toObject();
                    serverItems.push_back({item.value("adId").toInt(-1),
                                           item.value("title").toString(),
                                           item.value("category").toString(),
                                           item.value("priceTokens").toInt(0),
                                           item.value("sellerUsername").toString(),
                                           item.value("status").toString("approved")});
                }
                setItems(serverItems);
            });

    connect(AuthClient::instance(), &AuthClient::walletBalanceReceived, this,
            [this](bool success, const QString&, int balanceTokens) {
                if (success) {
                    walletBalanceTokens = balanceTokens;
                    setWalletBalanceLabel(balanceTokens);
                }
            });

    connect(AuthClient::instance(), &AuthClient::cartRemoveItemResultReceived, this,
            [this](bool success, const QString& message, int adId) {
                if (!success) {
                    QMessageBox::warning(this, "Cart", message);
                    return;
                }
                for (int i = 0; i < items.size(); ++i) {
                    if (items[i].adId == adId) {
                        items.removeAt(i);
                        break;
                    }
                }
                refreshCartTable();
                recomputeTotals();
            });

    connect(AuthClient::instance(), &AuthClient::cartClearResultReceived, this,
            [this](bool success, const QString& message) {
                if (!success) {
                    QMessageBox::warning(this, "Cart", message);
                    return;
                }
                items.clear();
                refreshCartTable();
                recomputeTotals();
            });

    connect(AuthClient::instance(), &AuthClient::buyResultReceived, this,
            [this](bool success, const QString& message, int balanceTokens, const QJsonArray&) {
                if (!success) {
                    QMessageBox::warning(this, "Purchase failed", message);
                    return;
                }

                walletBalanceTokens = balanceTokens;
                setWalletBalanceLabel(balanceTokens);
                ui->leDiscountCode->clear();
                ui->lblDiscountStatus->clear();
                refreshFromServer();
                QMessageBox::information(this, "Purchase", message);
                emit purchaseRequested(QString());
            });

    refreshFromServer();
}

cart_page::~cart_page()
{
    delete ui;
}

void cart_page::refreshFromServer()
{
    AuthClient* client = AuthClient::instance();
    client->sendMessage(client->withSession(common::Command::CartList));
    client->sendMessage(client->withSession(common::Command::WalletBalance));
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
        connect(btnRemove, &QPushButton::clicked, this, [this, row]() { removeItemAt(row); });
        ui->twCart->setCellWidget(row, 5, btnRemove);
    }
}

void cart_page::removeItemAt(int row)
{
    if (row < 0 || row >= items.size()) {
        return;
    }

    AuthClient::instance()->sendMessage(
        AuthClient::instance()->withSession(common::Command::CartRemoveItem,
                                            QJsonObject{{QStringLiteral("adId"), items[row].adId}}));
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
    AuthClient::instance()->sendMessage(AuthClient::instance()->withSession(common::Command::CartClear));
}

void cart_page::on_btnApplyDiscount_clicked()
{
    recomputeTotals();
    const QString code = ui->leDiscountCode->text().trimmed();
    if (code.isEmpty()) {
        ui->lblDiscountStatus->setText("Enter a discount code to apply.");
    } else if (discountTokens > 0) {
        ui->lblDiscountStatus->setText("Discount applied locally.");
    } else {
        ui->lblDiscountStatus->setText("Invalid or unsupported code.");
    }
}

void cart_page::on_btnPurchase_clicked()
{
    if (items.isEmpty()) {
        QMessageBox::information(this, "Purchase", "Your cart is empty.");
        return;
    }

    QJsonArray adIds;
    for (const auto& item : items) {
        adIds.append(item.adId);
    }

    AuthClient::instance()->sendMessage(
        AuthClient::instance()->withSession(common::Command::Buy,
                                            QJsonObject{{QStringLiteral("adIds"), adIds}}));
}
