#ifndef DISCOUNT_CODE_MANAGER_WINDOW_H
#define DISCOUNT_CODE_MANAGER_WINDOW_H

#include <QMainWindow>

class QCheckBox;
class QComboBox;
class QDateTimeEdit;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;
class WalletRepository;

class DiscountCodeManagerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit DiscountCodeManagerWindow(WalletRepository& walletRepository, QWidget* parent = nullptr);

private slots:
    void loadCodes();
    void saveCode();
    void deleteSelectedCode();
    void onRowSelected();
    void clearForm();

private:
    WalletRepository& walletRepository_;

    QTableWidget* table_ = nullptr;
    QLineEdit* codeEdit_ = nullptr;
    QComboBox* typeCombo_ = nullptr;
    QSpinBox* valueSpin_ = nullptr;
    QSpinBox* maxDiscountSpin_ = nullptr;
    QSpinBox* minSubtotalSpin_ = nullptr;
    QSpinBox* usageLimitSpin_ = nullptr;
    QCheckBox* unlimitedUsageCheck_ = nullptr;
    QCheckBox* activeCheck_ = nullptr;
    QCheckBox* expiresCheck_ = nullptr;
    QDateTimeEdit* expiresEdit_ = nullptr;
    QPushButton* deleteButton_ = nullptr;
};

#endif // DISCOUNT_CODE_MANAGER_WINDOW_H
