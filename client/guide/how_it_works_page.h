//
// Created by mamzi on 2/14/26.
//

#ifndef KALANET_HOW_IT_WORKS_PAGE_H
#define KALANET_HOW_IT_WORKS_PAGE_H

#include <QWidget>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class how_it_works_page; }
QT_END_NAMESPACE

class how_it_works_page : public QWidget {
    Q_OBJECT

public:
    explicit how_it_works_page(QWidget *parent = nullptr);
    ~how_it_works_page() override;

    signals:
        void backToMenuRequested();

private slots:
    void on_btnBackToMenu_clicked();
    void on_btnCopy_clicked();
    void on_btnExportTxt_clicked();
    void on_btnPrint_clicked();

private:
    QString guidePlainText() const;
    void setStatus(const QString& text);

private:
    Ui::how_it_works_page *ui;
};

#endif // KALANET_HOW_IT_WORKS_PAGE_H
