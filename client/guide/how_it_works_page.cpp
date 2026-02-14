//
// Created by mamzi on 2/14/26.
//

#include "how_it_works_page.h"
#include "ui_how_it_works_page.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

#ifdef QT_PRINTSUPPORT_LIB
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#endif

how_it_works_page::how_it_works_page(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::how_it_works_page)
{
    ui->setupUi(this);
    setStatus("");
}

how_it_works_page::~how_it_works_page()
{
    delete ui;
}

void how_it_works_page::setStatus(const QString &text)
{
    if (ui->lblStatus) ui->lblStatus->setText(text);
}

QString how_it_works_page::guidePlainText() const
{
    // Export/copy-friendly version of the guide
    return
        "PinkMart User Guide\n"
        "===================\n\n"
        "1) Create a New Ad\n"
        "- Open New Ad from Main Menu.\n"
        "- Fill Title, Category, Price, Description (image optional).\n"
        "- Submit: ad becomes Pending until admin approves.\n\n"
        "2) Shop\n"
        "- Shop shows only approved ads.\n"
        "- Use filters (name/category/price).\n"
        "- Add items to bucket list and remove them anytime.\n\n"
        "3) Cart & Purchase\n"
        "- Cart shows your bucket list.\n"
        "- Remove items individually or Remove all.\n"
        "- Apply discount code if you have one.\n"
        "- Purchase moves items to Profile -> Purchases.\n\n"
        "4) Profile\n"
        "- Update your account info.\n"
        "- Check wallet balance and add tokens (captcha may apply).\n"
        "- View Purchases and My Ads.\n\n"
        "Notes\n"
        "- Closing pages should return to Main Menu (use Back).\n"
        "- Some actions will be validated by server later.\n";
}

// -------------------- Slots (Qt auto-connect) --------------------

void how_it_works_page::on_btnBackToMenu_clicked()
{
    emit backToMenuRequested();
}

void how_it_works_page::on_btnCopy_clicked()
{
    const QString text = guidePlainText();
    QGuiApplication::clipboard()->setText(text);
    setStatus("Copied to clipboard.");
}

void how_it_works_page::on_btnExportTxt_clicked()
{
    const QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export guide as .txt",
        "PinkMart_Guide.txt",
        "Text Files (*.txt)"
    );

    if (filePath.isEmpty()) return;

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export failed", "Cannot write to the selected file.");
        return;
    }

    QTextStream out(&f);
    out << guidePlainText();
    f.close();

    setStatus("Exported successfully.");
}

void how_it_works_page::on_btnPrint_clicked()
{
#ifdef QT_PRINTSUPPORT_LIB
    QPrinter printer;
    QPrintDialog dialog(&printer, this);
    dialog.setWindowTitle("Print PinkMart Guide");

    if (dialog.exec() != QDialog::Accepted) return;

    // Print the QTextBrowser content
    if (ui->tbGuide) ui->tbGuide->print(&printer);

    setStatus("Print job sent.");
#else
    QMessageBox::information(this, "Print", "Qt PrintSupport module is not linked. Add Qt6::PrintSupport to enable printing.");
#endif
}
