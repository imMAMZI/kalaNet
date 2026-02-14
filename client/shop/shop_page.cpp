#include "shop_page.h"
#include "ui_shop_page.h"

#include "../network/auth_client.h"

#include <QHeaderView>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <utility>

shop_page::shop_page(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::shop_page)
{
    ui->setupUi(this);

    setupAdsTable();

    connect(AuthClient::instance(), &AuthClient::adListReceived, this,
            [this](bool success, const QString& message, const QJsonArray& ads) {
                if (!success) {
                    QMessageBox::warning(this, QStringLiteral("Shop"), message);
                    return;
                }

                allAds.clear();
                for (const QJsonValue& value : ads) {
                    const QJsonObject ad = value.toObject();
                    allAds.push_back({ad.value(QStringLiteral("id")).toInt(-1),
                                      ad.value(QStringLiteral("title")).toString(),
                                      ad.value(QStringLiteral("category")).toString(),
                                      ad.value(QStringLiteral("priceTokens")).toInt(0),
                                      ad.value(QStringLiteral("sellerUsername")).toString(),
                                      ad.value(QStringLiteral("status")).toString()});
                }

                filteredIndices.clear();
                for (int i = 0; i < allAds.size(); ++i) {
                    if (passesFilters(allAds[i])) {
                        filteredIndices.push_back(i);
                    }
                }
                refreshAdsTable();
            });

    connect(AuthClient::instance(), &AuthClient::cartListReceived, this,
            [this](bool success, const QString&, const QJsonArray& items) {
                if (!success) {
                    return;
                }

                cartPreviewItems.clear();
                for (const QJsonValue& value : items) {
                    const QJsonObject item = value.toObject();
                    cartPreviewItems.push_back({item.value(QStringLiteral("adId")).toInt(-1),
                                                item.value(QStringLiteral("title")).toString(),
                                                item.value(QStringLiteral("priceTokens")).toInt(0)});
                }
                refreshCartPreview();
            });

    connect(AuthClient::instance(), &AuthClient::cartAddItemResultReceived, this,
            [this](bool success, const QString& message, int, bool) {
                if (!success) {
                    QMessageBox::warning(this, QStringLiteral("Cart"), message);
                    return;
                }
                fetchCartFromServer();
            });

    connect(AuthClient::instance(), &AuthClient::adStatusNotifyReceived, this,
            [this](const QJsonArray&, const QString&) {
                fetchAdsFromServer();
                fetchCartFromServer();
            });

    fetchAdsFromServer();
    fetchCartFromServer();
}

shop_page::~shop_page()
{
    delete ui;
}

void shop_page::setupAdsTable()
{
    ui->twAds->setColumnCount(6);

    ui->twAds->horizontalHeader()->setStretchLastSection(true);
    ui->twAds->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->twAds->verticalHeader()->setVisible(false);

    ui->twAds->setColumnWidth(0, 90);
    ui->twAds->setColumnWidth(1, 220);
    ui->twAds->setColumnWidth(2, 140);
    ui->twAds->setColumnWidth(3, 90);
    ui->twAds->setColumnWidth(4, 140);
    ui->twAds->setColumnWidth(5, 140);

    ui->twAds->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->twAds->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->twAds->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void shop_page::refreshAdsTable()
{
    ui->twAds->setRowCount(filteredIndices.size());

    for (int row = 0; row < filteredIndices.size(); ++row) {
        const ShopItem& ad = allAds[filteredIndices[row]];

        auto *imgItem = new QTableWidgetItem(ad.adId > 0 ? QString::number(ad.adId) : QStringLiteral("-"));
        imgItem->setTextAlignment(Qt::AlignCenter);
        ui->twAds->setItem(row, 0, imgItem);

        ui->twAds->setItem(row, 1, new QTableWidgetItem(ad.title));
        ui->twAds->setItem(row, 2, new QTableWidgetItem(ad.category));
        ui->twAds->setItem(row, 3, new QTableWidgetItem(QString::number(ad.priceTokens) + QStringLiteral(" token")));
        ui->twAds->setItem(row, 4, new QTableWidgetItem(ad.seller));

        auto *btn = new QPushButton(QStringLiteral("Add"));
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(34);

        connect(btn, &QPushButton::clicked, this, [this, ad]() {
            if (ad.adId <= 0) {
                QMessageBox::warning(this, QStringLiteral("Cart"), QStringLiteral("Invalid ad id."));
                return;
            }

            AuthClient::instance()->sendMessage(
                AuthClient::instance()->withSession(common::Command::CartAddItem,
                                                    QJsonObject{{QStringLiteral("adId"), ad.adId}}));
        });

        ui->twAds->setCellWidget(row, 5, btn);
    }
}

void shop_page::refreshCartPreview()
{
    ui->lwBucket->clear();

    int total = 0;
    for (const CartPreviewItem& item : std::as_const(cartPreviewItems)) {
        total += item.priceTokens;

        auto *listItem = new QListWidgetItem(ui->lwBucket);
        auto *rowWidget = new QWidget(ui->lwBucket);
        auto *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(10, 6, 10, 6);
        rowLayout->setSpacing(10);

        auto *lbl = new QLabel(item.title + QStringLiteral(" — ") + QString::number(item.priceTokens) + QStringLiteral(" token"), rowWidget);
        lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        rowLayout->addWidget(lbl);

        listItem->setSizeHint(QSize(0, 40));
        ui->lwBucket->addItem(listItem);
        ui->lwBucket->setItemWidget(listItem, rowWidget);
    }

    ui->lblBucketTotal->setText(QString::number(total) + QStringLiteral(" token"));
}

QJsonObject shop_page::buildAdListPayload() const
{
    QJsonObject payload;
    payload.insert(QStringLiteral("name"), ui->leSearchName->text().trimmed());

    const QString selectedCategory = ui->cbCategory->currentText().trimmed();
    if (!selectedCategory.isEmpty() && selectedCategory != QStringLiteral("All categories")) {
        payload.insert(QStringLiteral("category"), selectedCategory);
    }

    payload.insert(QStringLiteral("minPriceTokens"), ui->sbMinPrice->value());
    payload.insert(QStringLiteral("maxPriceTokens"), ui->sbMaxPrice->value());
    payload.insert(QStringLiteral("sortBy"), QStringLiteral("createdAt"));
    payload.insert(QStringLiteral("sortOrder"), QStringLiteral("desc"));
    return payload;
}

bool shop_page::passesFilters(const ShopItem& ad) const
{
    const QString nameQuery = ui->leSearchName->text().trimmed();
    if (!nameQuery.isEmpty() && !ad.title.contains(nameQuery, Qt::CaseInsensitive)) {
        return false;
    }

    const QString selectedCategory = ui->cbCategory->currentText().trimmed();
    if (!selectedCategory.isEmpty() && selectedCategory != QStringLiteral("All categories") && ad.category != selectedCategory) {
        return false;
    }

    const int minPrice = ui->sbMinPrice->value();
    const int maxPrice = ui->sbMaxPrice->value();
    if (ad.priceTokens < minPrice) {
        return false;
    }
    if (maxPrice > 0 && ad.priceTokens > maxPrice) {
        return false;
    }

    return true;
}

void shop_page::fetchAdsFromServer()
{
    AuthClient::instance()->sendMessage(
        AuthClient::instance()->withSession(common::Command::AdList, buildAdListPayload()));
}

void shop_page::fetchCartFromServer()
{
    AuthClient::instance()->sendMessage(AuthClient::instance()->withSession(common::Command::CartList));
}

void shop_page::on_btnRefreshAds_clicked()
{
    fetchAdsFromServer();
}

void shop_page::on_btnApplyFilter_clicked()
{
    fetchAdsFromServer();
}

void shop_page::on_btnClearFilter_clicked()
{
    ui->leSearchName->clear();
    ui->cbCategory->setCurrentIndex(0);
    ui->sbMinPrice->setValue(0);
    ui->sbMaxPrice->setValue(0);
    fetchAdsFromServer();
}

void shop_page::on_btnGoToCart_clicked()
{
    emit goToCartRequested();
}

void shop_page::on_btnBackToMenu_clicked()
{
    emit backToMenuRequested();
}
