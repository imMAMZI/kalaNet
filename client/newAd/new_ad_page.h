//
// Created by mamzi on 2/14/26.
//

#ifndef KALANET_NEW_AD_PAGE_H
#define KALANET_NEW_AD_PAGE_H

#include <QWidget>
#include <QString>
#include <QPixmap>

QT_BEGIN_NAMESPACE
namespace Ui { class new_ad_page; }
QT_END_NAMESPACE

class new_ad_page : public QWidget {
    Q_OBJECT

public:
    explicit new_ad_page(QWidget *parent = nullptr);
    ~new_ad_page() override;

    signals:
        void backToMenuRequested();

    // Later (when common/server ready): send a request to create an ad
    // imageBytes can be empty if user didn't choose an image
    void submitAdRequested(const QString& title,
                           const QString& description,
                           const QString& category,
                           int priceTokens,
                           const QByteArray& imageBytes);

private slots:
    void on_btnBackToMenu_clicked();
    void on_btnChooseImage_clicked();
    void on_btnRemoveImage_clicked();
    void on_btnClearForm_clicked();
    void on_btnSubmitAd_clicked();

private:
    void wireLivePreview();
    void updatePreview();
    void setStatus(const QString& text);

    void clearForm();
    bool validateForm(QString& errorOut) const;

    QByteArray readFileBytes(const QString& filePath) const;
    void setImagePreviewFromBytes(const QByteArray& bytes);
    void setImagePreviewPlaceholder();

private:
    Ui::new_ad_page *ui;

    QString selectedImagePath;
    QByteArray selectedImageBytes;
    QPixmap selectedPixmap;
};

#endif // KALANET_NEW_AD_PAGE_H
