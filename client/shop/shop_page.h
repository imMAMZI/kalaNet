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
    explicit shop_page(QWidget *parent = nullptr);
    ~shop_page() override;

    signals:
        void goToCartRequested();
        void backToMenuRequested();
        void bucketChanged(int itemCount, int totalTokens);

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
    void loadDummyAds();              // placeholder until server/common is ready
    void refreshAdsTable();           // draw current ads into twAds

    void applyFilters();
    bool passesFilters(const AdRow& ad) const;

    void addAdToBucket(int adIndex);
    void removeAdFromBucketByAdIndex(int adIndex);
    void updateBucketUI();
    int bucketTotalTokens() const;

    void clearTable();
    void setBucketTotalLabel(int totalTokens);

private:
    Ui::shop_page *ui;

    QVector<AdRow> allAds;
    QVector<int> filteredIndices;     // indices into allAds after filters
    QVector<int> bucketIndices;       // indices into allAds that are in bucket
};

#endif // KALANET_SHOP_PAGE_H
