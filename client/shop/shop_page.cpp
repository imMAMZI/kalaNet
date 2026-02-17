#include "shop_page.h"
#include "ui_shop_page.h"

#include "../network/auth_client.h"

#include <QDialog>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPixmap>
#include <QPushButton>
#include <QTableWidgetItem>
#include <QVBoxLayout>
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
                adDetails.clear();
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

    connect(AuthClient::instance(), &AuthClient::adDetailResultReceived, this,
            [this](bool success, const QString& message, const QJsonObject& ad) {
                const int adId = ad.value(QStringLiteral("id")).toInt(-1);
                if (adId <= 0) {
                    return;
                }

                AdDetailData& detail = adDetails[adId];
                detail.requested = false;

                if (!success) {
                    if (pendingPreviewAdId == adId) {
                        pendingPreviewAdId = -1;
                        QMessageBox::warning(this, QStringLiteral("Preview"),
                                             message.isEmpty() ? QStringLiteral("Could not load ad preview") : message);
                    }
                    return;
                }

                detail.loaded = true;
                detail.description = ad.value(QStringLiteral("description")).toString();
                detail.imageBytes = QByteArray::fromBase64(ad.value(QStringLiteral("imageBase64")).toString().toLatin1());

                refreshAdsTable();

                if (pendingPreviewAdId == adId) {
                    pendingPreviewAdId = -1;
                    showAdPreviewDialog(adId);
                }
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

    connect(AuthClient::instance(), &AuthClient::cartRemoveItemResultReceived, this,
            [this](bool success, const QString&, int) {
                if (!success) {
                    return;
                }
                fetchCartFromServer();
            });

    connect(AuthClient::instance(), &AuthClient::cartClearResultReceived, this,
            [this](bool success, const QString&) {
                if (!success) {
                    return;
                }
                fetchCartFromServer();
            });

    connect(AuthClient::instance(), &AuthClient::buyResultReceived, this,
            [this](bool success, const QString&, int, const QJsonArray&) {
                if (!success) {
                    return;
                }
                refreshFromServer();
            });

    connect(AuthClient::instance(), &AuthClient::adStatusNotifyReceived, this,
            [this](const QJsonArray&, const QString&) {
                fetchAdsFromServer();
                fetchCartFromServer();
            });

    connect(ui->twAds, &QTableWidget::cellDoubleClicked, this,
            [this](int row, int) {
                if (row < 0 || row >= filteredIndices.size()) {
                    return;
                }
                const ShopItem& ad = allAds[filteredIndices[row]];
                showAdPreviewDialog(ad.adId);
            });

    refreshFromServer();
}

shop_page::~shop_page()
{
    delete ui;
}

void shop_page::setupAdsTable()
{
    ui->twAds->setColumnCount(7);
    ui->twAds->setHorizontalHeaderLabels({QStringLiteral("Preview"),
                                          QStringLiteral("Title"),
                                          QStringLiteral("Category"),
                                          QStringLiteral("Price"),
                                          QStringLiteral("Seller"),
                                          QStringLiteral("Review"),
                                          QStringLiteral("Add to cart")});

    ui->twAds->horizontalHeader()->setStretchLastSection(true);
    ui->twAds->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui->twAds->verticalHeader()->setVisible(false);

    ui->twAds->setColumnWidth(0, 90);
    ui->twAds->setColumnWidth(1, 220);
    ui->twAds->setColumnWidth(2, 140);
    ui->twAds->setColumnWidth(3, 90);
    ui->twAds->setColumnWidth(4, 140);
    ui->twAds->setColumnWidth(5, 120);
    ui->twAds->setColumnWidth(6, 140);

    ui->twAds->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->twAds->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->twAds->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->twAds->setAlternatingRowColors(true);
}

void shop_page::refreshAdsTable()
{
    ui->twAds->setRowCount(filteredIndices.size());

    for (int row = 0; row < filteredIndices.size(); ++row) {
        const ShopItem& ad = allAds[filteredIndices[row]];
        auto *imageLabel = new QLabel();
        imageLabel->setAlignment(Qt::AlignCenter);
        imageLabel->setMinimumSize(72, 54);

        const AdDetailData detail = adDetails.value(ad.adId);
        if (detail.loaded && !detail.imageBytes.isEmpty()) {
            QPixmap pixmap;
            if (pixmap.loadFromData(detail.imageBytes)) {
                imageLabel->setPixmap(pixmap.scaled(72, 54, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                imageLabel->setText(QStringLiteral("No image"));
            }
        } else {
            imageLabel->setText(QStringLiteral("Preview"));
            requestAdDetail(ad.adId);
        }
        ui->twAds->setCellWidget(row, 0, imageLabel);

        ui->twAds->setItem(row, 1, new QTableWidgetItem(ad.title));
        ui->twAds->setItem(row, 2, new QTableWidgetItem(ad.category));
        ui->twAds->setItem(row, 3, new QTableWidgetItem(QString::number(ad.priceTokens) + QStringLiteral(" token")));
        ui->twAds->setItem(row, 4, new QTableWidgetItem(ad.seller));

        auto *reviewBtn = new QPushButton(QStringLiteral("Review"));
        reviewBtn->setObjectName(QStringLiteral("btnReviewAd"));
        reviewBtn->setCursor(Qt::PointingHandCursor);
        reviewBtn->setMinimumHeight(34);
        connect(reviewBtn, &QPushButton::clicked, this, [this, ad]() {
            showAdPreviewDialog(ad.adId);
        });
        ui->twAds->setCellWidget(row, 5, reviewBtn);

        auto *addBtn = new QPushButton(QStringLiteral("Add"));
        addBtn->setObjectName(QStringLiteral("btnAddAd"));
        addBtn->setCursor(Qt::PointingHandCursor);
        addBtn->setMinimumHeight(34);

        connect(addBtn, &QPushButton::clicked, this, [this, ad]() {
            if (ad.adId <= 0) {
                QMessageBox::warning(this, QStringLiteral("Cart"), QStringLiteral("Invalid ad id."));
                return;
            }

            AuthClient::instance()->sendMessage(
                AuthClient::instance()->withSession(common::Command::CartAddItem,
                                                    QJsonObject{{QStringLiteral("adId"), ad.adId}}));
        });
        ui->twAds->setCellWidget(row, 6, addBtn);
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

        auto *removeBtn = new QPushButton(QStringLiteral("Remove"), rowWidget);
        removeBtn->setCursor(Qt::PointingHandCursor);
        removeBtn->setMinimumHeight(28);
        connect(removeBtn, &QPushButton::clicked, this, [this, adId = item.adId]() {
            if (adId <= 0) {
                return;
            }
            AuthClient::instance()->sendMessage(
                AuthClient::instance()->withSession(common::Command::CartRemoveItem,
                                                    QJsonObject{{QStringLiteral("adId"), adId}}));
        });
        rowLayout->addWidget(removeBtn);

        listItem->setSizeHint(QSize(0, 40));
        ui->lwBucket->addItem(listItem);
        ui->lwBucket->setItemWidget(listItem, rowWidget);
    }

    ui->lblBucketTotal->setText(QString::number(total) + QStringLiteral(" token"));
}

void shop_page::requestAdDetail(int adId)
{
    if (adId <= 0) {
        return;
    }

    AdDetailData& detail = adDetails[adId];
    if (detail.loaded || detail.requested) {
        return;
    }

    detail.requested = true;
    AuthClient::instance()->sendMessage(
        AuthClient::instance()->withSession(common::Command::AdDetail,
                                            QJsonObject{{QStringLiteral("adId"), adId}}));
}

void shop_page::showAdPreviewDialog(int adId)
{
    const int index = [&]() {
        for (int i = 0; i < allAds.size(); ++i) {
            if (allAds[i].adId == adId) {
                return i;
            }
        }
        return -1;
    }();

    if (index < 0) {
        return;
    }

    const AdDetailData detail = adDetails.value(adId);
    if (!detail.loaded) {
        pendingPreviewAdId = adId;
        requestAdDetail(adId);
        return;
    }

    const ShopItem& ad = allAds[index];
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("Ad preview"));
    dialog.resize(560, 700);
    dialog.setStyleSheet(QStringLiteral(
        "QDialog {"
        "  background: #090910;"
        "  color: #F8F8FF;"
        "}"
        "QFrame#previewCard {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #11111A, stop:1 #0D0D15);"
        "  border: 1px solid #2F2F43;"
        "  border-radius: 20px;"
        "}"
        "QLabel#previewTitle {"
        "  color: #FF58B6;"
        "  font-size: 24px;"
        "  font-weight: 900;"
        "}"
        "QLabel#previewMeta {"
        "  color: #D6D6E5;"
        "  font-size: 12px;"
        "  font-weight: 700;"
        "}"
        "QLabel#sectionTitle {"
        "  color: #9B9BB0;"
        "  font-size: 11px;"
        "  letter-spacing: 1px;"
        "  text-transform: uppercase;"
        "}"
        "QPlainTextEdit#previewDescription {"
        "  background: #0E0E14;"
        "  border: 1px solid #2A2A39;"
        "  border-radius: 12px;"
        "  color: #F8F8FF;"
        "  padding: 8px;"
        "}"
        "QPushButton#closePreviewBtn {"
        "  background: #FF3DA5;"
        "  color: #0B0B0F;"
        "  border: none;"
        "  border-radius: 12px;"
        "  padding: 10px 16px;"
        "  font-weight: 800;"
        "}"
        "QPushButton#closePreviewBtn:hover { background: #FF63B7; }"
        "QPushButton#closePreviewBtn:pressed { background: #E92A93; }"));

    auto *dialogLayout = new QVBoxLayout(&dialog);
    dialogLayout->setContentsMargins(18, 18, 18, 18);

    auto *card = new QFrame(&dialog);
    card->setObjectName(QStringLiteral("previewCard"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    auto *image = new QLabel(card);
    image->setMinimumHeight(290);
    image->setAlignment(Qt::AlignCenter);
    image->setStyleSheet(QStringLiteral("background:#0A0A12; border:1px solid #323248; border-radius: 14px;"));
    if (!detail.imageBytes.isEmpty()) {
        QPixmap pix;
        if (pix.loadFromData(detail.imageBytes)) {
            image->setPixmap(pix.scaled(500, 290, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            image->setText(QStringLiteral("Image unavailable"));
        }
    } else {
        image->setText(QStringLiteral("No image provided"));
    }
    layout->addWidget(image);

    auto *title = new QLabel(ad.title, card);
    title->setObjectName(QStringLiteral("previewTitle"));
    title->setWordWrap(true);
    layout->addWidget(title);

    auto *meta = new QLabel(QStringLiteral("%1 token • %2")
                            .arg(ad.priceTokens)
                            .arg(ad.category),
                            card);
    meta->setObjectName(QStringLiteral("previewMeta"));
    layout->addWidget(meta);

    auto *seller = new QLabel(QStringLiteral("Seller: %1").arg(ad.seller), card);
    seller->setStyleSheet(QStringLiteral("color:#AFAFC4; font-size:11px;"));
    layout->addWidget(seller);

    auto *descriptionTitle = new QLabel(QStringLiteral("Description"), card);
    descriptionTitle->setObjectName(QStringLiteral("sectionTitle"));
    layout->addWidget(descriptionTitle);

    auto *desc = new QPlainTextEdit(card);
    desc->setObjectName(QStringLiteral("previewDescription"));
    desc->setReadOnly(true);
    desc->setPlainText(detail.description.trimmed().isEmpty()
                           ? QStringLiteral("No description provided")
                           : detail.description);
    layout->addWidget(desc, 1);

    dialogLayout->addWidget(card, 1);

    auto *closeBtn = new QPushButton(QStringLiteral("Close"), &dialog);
    closeBtn->setObjectName(QStringLiteral("closePreviewBtn"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    dialogLayout->addWidget(closeBtn, 0, Qt::AlignRight);
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
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

void shop_page::refreshFromServer()
{
    fetchAdsFromServer();
    fetchCartFromServer();
}

void shop_page::on_btnRefreshAds_clicked()
{
    refreshFromServer();
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
