#ifndef KALANET_CART_PAGE_H
#define KALANET_CART_PAGE_H

#include <QWidget>
#include <QString>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui {
    class cart_page;
}
QT_END_NAMESPACE

class cart_page : public QWidget {
    Q_OBJECT

public:
    struct CartItemData {
        QString title;
        QString category;
        int priceTokens = 0;
        QString seller;
        QString status;
    };

    explicit cart_page(QWidget *parent = nullptr);
    ~cart_page() override;
    void setItems(const QVector<CartItemData>& newItems);
    QVector<CartItemData> itemsData() const;

    signals:
        void backToMenuRequested();
        void purchaseRequested(const QString& discountCode);
        void cartChanged(int itemCount, int subtotalTokens, int discountTokens, int totalTokens);
        void cartItemsChanged(const QVector<CartItemData>& items);
        void purchaseCompleted(const QVector<CartItemData>& purchasedItems, const QString& discountCode);

private slots:
    void on_btnBackToMenu_clicked();
    void on_btnClearAll_clicked();
    void on_btnApplyDiscount_clicked();
    void on_btnPurchase_clicked();

private:
    struct CartItem {
        QString title;
        QString category;
        int priceTokens = 0;
        QString seller;
        QString status; // e.g., "Available" (later: sold/unavailable)
    };

    void setupCartTable();
    void loadDummyCart();         // placeholder until you connect with Shop/common
    void refreshCartTable();
    void recomputeTotals();

    void removeItemAt(int row);
    void clearAll();
    void notifyItemsChanged();

    int computeDiscountTokens(const QString& code, int subtotal) const;

    void setWalletBalanceLabel(int tokens);
    void setTotalsLabels(int subtotal, int discount, int total);

private:
    Ui::cart_page *ui;

    QVector<CartItemData> items;

    int walletBalanceTokens = 0;
    int subtotalTokens = 0;
    int discountTokens = 0;
    int totalTokens = 0;
};

#endif // KALANET_CART_PAGE_H
