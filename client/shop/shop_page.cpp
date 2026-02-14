//
// Created by mamzi on 2/14/26.
//

#include "shop_page.h"
#include "ui_shop_page.h"

#include <QHeaderView>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>

shop_page::shop_page(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::shop_page)
{
    ui->setupUi(this);

    setupAdsTable();
    loadDummyAds();

    applyFilters();
    refreshAdsTable();
    updateBucketUI();
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

    ui->twAds->setColumnWidth(0, 90);   // Image
    ui->twAds->setColumnWidth(1, 220);  // Title
    ui->twAds->setColumnWidth(2, 140);  // Category
    ui->twAds->setColumnWidth(3, 90);   // Price
    ui->twAds->setColumnWidth(4, 140);  // Seller
    ui->twAds->setColumnWidth(5, 140);  // Action

    ui->twAds->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->twAds->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->twAds->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void shop_page::loadDummyAds()
{
    allAds.clear();
    allAds.push_back({"iPhone 11", "Electronics", 120, "Ali"});
    allAds.push_back({"Wood Chair", "Home", 35, "Sara"});
    allAds.push_back({"Hoodie XL", "Clothing", 25, "Reza"});
    allAds.push_back({"C++ Book", "Books", 18, "Neda"});
    allAds.push_back({"Headphones", "Electronics", 55, "Mehdi"});
}

void shop_page::clearTable()
{
    ui->twAds->setRowCount(0);
}

void shop_page::refreshAdsTable()
{
    clearTable();

    ui->twAds->setRowCount(static_cast<int>(filteredIndices.size()));

    for (int row = 0; row < static_cast<int>(filteredIndices.size()); ++row) {
        const int adIndex = filteredIndices[row];
        const AdRow& ad = allAds[adIndex];

        // Image placeholder
        auto *imgItem = new QTableWidgetItem("IMG");
        imgItem->setTextAlignment(Qt::AlignCenter);
        ui->twAds->setItem(row, 0, imgItem);

        ui->twAds->setItem(row, 1, new QTableWidgetItem(ad.title));
        ui->twAds->setItem(row, 2, new QTableWidgetItem(ad.category));
        ui->twAds->setItem(row, 3, new QTableWidgetItem(QString::number(ad.priceTokens) + " token"));
        ui->twAds->setItem(row, 4, new QTableWidgetItem(ad.seller));

        // Add button
        auto *btn = new QPushButton("Add");
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(34);
        btn->setStyleSheet(
            "QPushButton{background:#FF3DA5;color:#0B0B0F;border:none;border-radius:12px;padding:6px 10px;font-weight:800;}"
            "QPushButton:hover{background:#FF63B7;}"
            "QPushButton:pressed{background:#E92A93;}"
        );

        connect(btn, &QPushButton::clicked, this, [this, adIndex]() {
            addAdToBucket(adIndex);
        });

        ui->twAds->setCellWidget(row, 5, btn);
    }
}

void shop_page::applyFilters()
{
    filteredIndices.clear();
    for (int i = 0; i < static_cast<int>(allAds.size()); ++i) {
        if (passesFilters(allAds[i])) {
            filteredIndices.push_back(i);
        }
    }
}

bool shop_page::passesFilters(const AdRow& ad) const
{
    const QString nameQuery = ui->leSearchName->text().trimmed();
    const QString selectedCat = ui->cbCategory->currentText().trimmed();

    const int minPrice = static_cast<int>(ui->sbMinPrice->value());
    const int maxPrice = static_cast<int>(ui->sbMaxPrice->value()); // 0 means "no max"

    if (!nameQuery.isEmpty() && !ad.title.contains(nameQuery, Qt::CaseInsensitive)) {
        return false;
    }

    if (!selectedCat.isEmpty() && selectedCat != "All categories") {
        if (ad.category != selectedCat) {
            return false;
        }
    }

    if (ad.priceTokens < minPrice) return false;
    if (maxPrice > 0 && ad.priceTokens > maxPrice) return false;

    return true;
}

void shop_page::addAdToBucket(int adIndex)
{
    for (int idx : bucketIndices) {
        if (idx == adIndex) {
            QMessageBox::information(this, "Bucket list", "This item is already in your bucket list.");
            return;
        }
    }

    bucketIndices.push_back(adIndex);
    updateBucketUI();
}

void shop_page::removeAdFromBucketByAdIndex(int adIndex)
{
    for (int i = 0; i < static_cast<int>(bucketIndices.size()); ++i) {
        if (bucketIndices[i] == adIndex) {
            bucketIndices.removeAt(i);
            updateBucketUI();
            return;
        }
    }
}

int shop_page::bucketTotalTokens() const
{
    int total = 0;
    for (int adIdx : bucketIndices) {
        total += allAds[adIdx].priceTokens;
    }
    return total;
}

void shop_page::setBucketTotalLabel(int totalTokens)
{
    ui->lblBucketTotal->setText(QString::number(totalTokens) + " token");
}

void shop_page::updateBucketUI()
{
    ui->lwBucket->clear();

    // Build each row as: [Title + price]   [Remove button]
    for (int i = 0; i < static_cast<int>(bucketIndices.size()); ++i) {
        const int adIndex = bucketIndices[i];
        const AdRow& ad = allAds[adIndex];

        // QListWidgetItem holder
        auto *item = new QListWidgetItem(ui->lwBucket);

        // Custom row widget
        auto *rowWidget = new QWidget(ui->lwBucket);
        auto *rowLayout = new QHBoxLayout(rowWidget);
        rowLayout->setContentsMargins(10, 6, 10, 6);
        rowLayout->setSpacing(10);

        auto *lbl = new QLabel(ad.title + " — " + QString::number(ad.priceTokens) + " token", rowWidget);
        lbl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto *btnRemove = new QPushButton("Remove", rowWidget);
        btnRemove->setCursor(Qt::PointingHandCursor);
        btnRemove->setMinimumHeight(34);
        btnRemove->setStyleSheet(
            "QPushButton{background:transparent;color:#FF5C7A;border:1px solid #FF5C7A;border-radius:12px;padding:6px 10px;font-weight:900;}"
            "QPushButton:hover{background:rgba(255,92,122,0.12);}"
            "QPushButton:pressed{background:rgba(255,92,122,0.20);}"
        );

        connect(btnRemove, &QPushButton::clicked, this, [this, adIndex]() {
            removeAdFromBucketByAdIndex(adIndex);
        });

        rowLayout->addWidget(lbl);
        rowLayout->addWidget(btnRemove);

        rowWidget->setLayout(rowLayout);

        // Make row height nice
        item->setSizeHint(QSize(0, 46));
        ui->lwBucket->addItem(item);
        ui->lwBucket->setItemWidget(item, rowWidget);
    }

    const int total = bucketTotalTokens();
    setBucketTotalLabel(total);
    emit bucketChanged(bucketIndices.size(), total);
}

// -------------------- Slots (Qt auto-connect) --------------------

void shop_page::on_btnRefreshAds_clicked()
{
    loadDummyAds();
    applyFilters();
    refreshAdsTable();
}

void shop_page::on_btnApplyFilter_clicked()
{
    applyFilters();
    refreshAdsTable();
}

void shop_page::on_btnClearFilter_clicked()
{
    ui->leSearchName->clear();
    ui->cbCategory->setCurrentIndex(0);
    ui->sbMinPrice->setValue(0);
    ui->sbMaxPrice->setValue(0);

    applyFilters();
    refreshAdsTable();
}

void shop_page::on_btnGoToCart_clicked()
{
    emit goToCartRequested();
}

void shop_page::on_btnBackToMenu_clicked()
{
    emit backToMenuRequested();
}