//
// Created by mamzi on 2/14/26.
//

#ifndef KALANET_SHOP_PAGE_H
#define KALANET_SHOP_PAGE_H

#include <QWidget>
#include <QString>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui {
    class shop_page;
}
QT_END_NAMESPACE

class shop_page : public QWidget {
    Q_OBJECT

public:
    struct ShopItem {
        QString title;
        QString category;
        int priceTokens = 0;
        QString seller;
    };

    explicit shop_page(QWidget *parent = nullptr);
    ~shop_page() override;

    QVector<ShopItem> bucketItems() const;
    void setBucketItems(const QVector<ShopItem>& items);
    void markItemsAsPurchased(const QVector<ShopItem>& purchasedItems);

    signals:
        void goToCartRequested();
        void backToMenuRequested();
        void bucketChanged(int itemCount, int totalTokens);
        void bucketItemsChanged(const QVector<ShopItem>& items);

private slots:
    void on_btnRefreshAds_clicked();
    void on_btnApplyFilter_clicked();
    void on_btnClearFilter_clicked();
    void on_btnGoToCart_clicked();
    void on_btnBackToMenu_clicked();

private:
    struct AdRow {
        QString title;
        QString category;
        int priceTokens = 0;
        QString seller;
    };

    void setupAdsTable();
    void loadDummyAds();
    void refreshAdsTable();

    void applyFilters();
    bool passesFilters(const ShopItem& ad) const;
    static bool sameItem(const ShopItem& a, const ShopItem& b);

    void addAdToBucket(int adIndex);
    void removeAdFromBucketByAdIndex(int adIndex);
    void updateBucketUI();
    int bucketTotalTokens() const;

    void clearTable();
    void setBucketTotalLabel(int totalTokens);

private:
    Ui::shop_page *ui;

    QVector<ShopItem> allAds;
    QVector<int> filteredIndices;
    QVector<int> bucketIndices;
};

#endif // KALANET_SHOP_PAGE_H
