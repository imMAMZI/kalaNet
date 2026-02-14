#ifndef KALANET_SHOP_PAGE_H
#define KALANET_SHOP_PAGE_H

#include <QWidget>
#include <QString>
#include <QVector>

#include <QJsonObject>

QT_BEGIN_NAMESPACE
namespace Ui {
    class shop_page;
}
QT_END_NAMESPACE

class shop_page : public QWidget {
    Q_OBJECT

public:
    struct ShopItem {
        int adId = -1;
        QString title;
        QString category;
        int priceTokens = 0;
        QString seller;
        QString status;
    };

    explicit shop_page(QWidget *parent = nullptr);
    ~shop_page() override;

signals:
    void goToCartRequested();
    void backToMenuRequested();

private slots:
    void on_btnRefreshAds_clicked();
    void on_btnApplyFilter_clicked();
    void on_btnClearFilter_clicked();
    void on_btnGoToCart_clicked();
    void on_btnBackToMenu_clicked();

private:
    struct CartPreviewItem {
        int adId = -1;
        QString title;
        int priceTokens = 0;
    };

    void setupAdsTable();
    void refreshAdsTable();
    void refreshCartPreview();
    void fetchAdsFromServer();
    void fetchCartFromServer();
    QJsonObject buildAdListPayload() const;
    bool passesFilters(const ShopItem& ad) const;

private:
    Ui::shop_page *ui;

    QVector<ShopItem> allAds;
    QVector<int> filteredIndices;
    QVector<CartPreviewItem> cartPreviewItems;
};

#endif // KALANET_SHOP_PAGE_H
