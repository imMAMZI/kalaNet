//
// Created by mamzi on 2/14/26.
//

#include "new_ad_page.h"
#include "ui_new_ad_page.h"

#include <QFileDialog>
#include <QFile>
#include <QMessageBox>

new_ad_page::new_ad_page(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::new_ad_page)
{
    ui->setupUi(this);

    // Set initial preview defaults
    setImagePreviewPlaceholder();
    updatePreview();
    setStatus("");

    wireLivePreview();
}

new_ad_page::~new_ad_page()
{
    delete ui;
}

void new_ad_page::wireLivePreview()
{
    // Update preview whenever inputs change
    connect(ui->leAdTitle, &QLineEdit::textChanged, this, &new_ad_page::updatePreview);
    connect(ui->cbCategory, &QComboBox::currentTextChanged, this, &new_ad_page::updatePreview);
    connect(ui->sbPrice, QOverload<int>::of(&QSpinBox::valueChanged), this, &new_ad_page::updatePreview);

    connect(ui->pteDescription, &QPlainTextEdit::textChanged, this, &new_ad_page::updatePreview);
}

void new_ad_page::updatePreview()
{
    const QString title = ui->leAdTitle->text().trimmed();
    const QString cat   = ui->cbCategory->currentText();
    const int price     = ui->sbPrice->value();
    const QString desc  = ui->pteDescription->toPlainText().trimmed();

    ui->lblPreviewTitle->setText(title.isEmpty() ? "Title will appear here" : title);
    ui->lblPreviewCategory->setText(QString("Category: %1").arg(cat.isEmpty() ? "-" : cat));
    ui->lblPreviewPrice->setText(QString("Price: %1 token").arg(price));

    if (desc.isEmpty())
        ui->lblPreviewDesc->setText("Description will appear here…");
    else
        ui->lblPreviewDesc->setText(desc);
}

void new_ad_page::setStatus(const QString &text)
{
    if (ui->lblStatus) ui->lblStatus->setText(text);
}

void new_ad_page::setImagePreviewPlaceholder()
{
    selectedImagePath.clear();
    selectedImageBytes.clear();
    selectedPixmap = QPixmap();

    ui->lblImagePreview->setText("Image preview");
    ui->lblImagePreview->setPixmap(QPixmap()); // clear pixmap if any
    ui->lblImagePreview->setScaledContents(true);
}

QByteArray new_ad_page::readFileBytes(const QString &filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return {};
    return f.readAll();
}

void new_ad_page::setImagePreviewFromBytes(const QByteArray &bytes)
{
    if (bytes.isEmpty()) {
        setImagePreviewPlaceholder();
        return;
    }

    QPixmap pm;
    if (!pm.loadFromData(bytes)) {
        setStatus("Could not load image. Please choose a valid PNG/JPG file.");
        setImagePreviewPlaceholder();
        return;
    }

    selectedPixmap = pm;
    ui->lblImagePreview->setText("");
    ui->lblImagePreview->setPixmap(selectedPixmap);
    ui->lblImagePreview->setScaledContents(true);
}

bool new_ad_page::validateForm(QString &errorOut) const
{
    const QString title = ui->leAdTitle->text().trimmed();
    const QString desc  = ui->pteDescription->toPlainText().trimmed();
    const QString cat   = ui->cbCategory->currentText().trimmed();
    const int price     = ui->sbPrice->value();

    if (title.isEmpty()) { errorOut = "Title is required."; return false; }
    if (title.size() < 3) { errorOut = "Title is too short (min 3 characters)."; return false; }

    if (cat.isEmpty()) { errorOut = "Category is required."; return false; }

    if (price <= 0) { errorOut = "Price must be at least 1 token."; return false; }

    if (desc.isEmpty()) { errorOut = "Description is required."; return false; }
    if (desc.size() < 10) { errorOut = "Description is too short (min 10 characters)."; return false; }

    return true;
}

void new_ad_page::clearForm()
{
    ui->leAdTitle->clear();
    ui->cbCategory->setCurrentIndex(0);
    ui->sbPrice->setValue(10);
    ui->pteDescription->clear();

    setImagePreviewPlaceholder();
    updatePreview();
    setStatus("Form cleared.");
}

void new_ad_page::on_btnBackToMenu_clicked()
{
    emit backToMenuRequested();
}

void new_ad_page::on_btnChooseImage_clicked()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Choose image",
        QString(),
        "Images (*.png *.jpg *.jpeg)"
    );

    if (filePath.isEmpty()) return;

    const QByteArray bytes = readFileBytes(filePath);
    if (bytes.isEmpty()) {
        setStatus("Could not read the selected file.");
        return;
    }

    selectedImagePath = filePath;
    selectedImageBytes = bytes;

    setImagePreviewFromBytes(selectedImageBytes);
    setStatus("Image selected.");
}

void new_ad_page::on_btnRemoveImage_clicked()
{
    setImagePreviewPlaceholder();
    setStatus("Image removed.");
}

void new_ad_page::on_btnClearForm_clicked()
{
    clearForm();
}

void new_ad_page::on_btnSubmitAd_clicked()
{
    QString err;
    if (!validateForm(err)) {
        setStatus(err);
        QMessageBox::warning(this, "Invalid input", err);
        return;
    }

    const QString title = ui->leAdTitle->text().trimmed();
    const QString desc  = ui->pteDescription->toPlainText().trimmed();
    const QString cat   = ui->cbCategory->currentText().trimmed();
    const int price     = ui->sbPrice->value();

    emit submitAdRequested(title, desc, cat, price, selectedImageBytes);
    setStatus("Submitting ad to server...");
}
