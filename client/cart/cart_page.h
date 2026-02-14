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
        int adId = -1;
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
    void refreshFromServer();

signals:
    void backToMenuRequested();
    void purchaseRequested(const QString& discountCode);

private slots:
    void on_btnBackToMenu_clicked();
    void on_btnClearAll_clicked();
    void on_btnApplyDiscount_clicked();
    void on_btnPurchase_clicked();

private:
    void setupCartTable();
    void refreshCartTable();
    void recomputeTotals();
    void removeItemAt(int row);
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
