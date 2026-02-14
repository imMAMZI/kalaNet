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
    explicit cart_page(QWidget *parent = nullptr);
    ~cart_page() override;

    signals:
        void backToMenuRequested();
    void purchaseRequested(const QString& discountCode);

    // Optional: let main window/state know changes
    void cartChanged(int itemCount, int subtotalTokens, int discountTokens, int totalTokens);

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
    void refreshCartTable();      // render rows + remove buttons
    void recomputeTotals();

    void removeItemAt(int row);
    void clearAll();

    // discount logic (local preview only for now)
    int computeDiscountTokens(const QString& code, int subtotal) const;

    void setWalletBalanceLabel(int tokens);
    void setTotalsLabels(int subtotal, int discount, int total);

private:
    Ui::cart_page *ui;

    QVector<CartItem> items;

    int walletBalanceTokens = 0; // placeholder until profile/common ready
    int subtotalTokens = 0;
    int discountTokens = 0;
    int totalTokens = 0;
};

#endif // KALANET_CART_PAGE_H
