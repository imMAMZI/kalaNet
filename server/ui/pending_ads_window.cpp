#include "pending_ads_window.h"

#include <QDateTime>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QPixmap>
#include <QSignalBlocker>
#include <QSet>
#include <QStandardItemModel>

#include "ui_pending_ads_window.h"

namespace {
QString prettifyStatus(const QString& status)
{
    const QString normalized = status.trimmed().toLower();
    if (normalized == QStringLiteral("pending")) {
        return QObject::tr("Pending");
    }
    if (normalized == QStringLiteral("approved")) {
        return QObject::tr("Approved");
    }
    if (normalized == QStringLiteral("rejected")) {
        return QObject::tr("Rejected");
    }
    if (normalized == QStringLiteral("sold")) {
        return QObject::tr("Sold");
    }
    if (normalized == QStringLiteral("archived")) {
        return QObject::tr("Archived");
    }
    return status;
}

AdRepository::AdModerationStatus parseAction(const QString& action)
{
    const QString normalized = action.trimmed().toLower();
    if (normalized == QStringLiteral("approve")) {
        return AdRepository::AdModerationStatus::Approved;
    }
    if (normalized == QStringLiteral("reject")) {
        return AdRepository::AdModerationStatus::Rejected;
    }
    return AdRepository::AdModerationStatus::Unknown;
}
} // namespace

PendingAdsWindow::PendingAdsWindow(AdRepository& adRepository, QWidget* parent)
    : QMainWindow(parent)
    , adRepository_(adRepository)
    , ui(new Ui::PendingAdsWindow)
{
    ui->setupUi(this);
    ui->splitterMain->setStretchFactor(0, 2);
    ui->splitterMain->setStretchFactor(1, 3);
    ui->splitterMain->setSizes({560, 760});

    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget { background-color: #1f232b; color: #e7ebf3; }"
        "QGroupBox {"
        "  border: 1px solid #424a5a;"
        "  border-radius: 8px;"
        "  margin-top: 12px;"
        "  background-color: #2a303b;"
        "  font-weight: 600;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 12px;"
        "  padding: 0 6px;"
        "  color: #f1f4fb;"f
        "}"
        "QTableView, QTableWidget, QTextBrowser, QPlainTextEdit {"
        "  background-color: #252b36;"
        "  color: #e7ebf3;"
        "  border: 1px solid #465064;"
        "  border-radius: 6px;"
        "  selection-background-color: #3f6eb8;"
        "  selection-color: #ffffff;"
        "}"
        "QHeaderView::section {"
        "  background-color: #333b49;"
        "  color: #eaf0fa;"
        "  padding: 5px;"
        "  border: none;"
        "  border-right: 1px solid #4b5568;"
        "}"
        "QPushButton {"
        "  border: 1px solid #5b6680;"
        "  border-radius: 6px;"
        "  padding: 6px 10px;"
        "  background-color: #353d4c;"
        "  color: #f1f4fb;"
        "}"
        "QPushButton:hover { background-color: #414c61; }"
        "QLineEdit, QComboBox, QSpinBox {"
        "  border: 1px solid #5b6680;"
        "  border-radius: 6px;"
        "  padding: 4px 6px;"
        "  background-color: #202632;"
        "  color: #eef3ff;"
        "}"));

    auto* model = new QStandardItemModel(this);
    model->setColumnCount(AdTableColumns::Count);
    model->setHorizontalHeaderLabels({
        tr("Id"), tr("Title"), tr("Category"), tr("Price"), tr("Seller"),
        tr("Status"), tr("Created"), tr("Updated"), tr("Has Image")
    });
    ui->tableViewPendingAds->setModel(model);
    ui->tableViewPendingAds->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewPendingAds->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewPendingAds->horizontalHeader()->setStretchLastSection(true);
    ui->tableViewPendingAds->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->tableViewPendingAds->setSortingEnabled(true);
    ui->tableViewPendingAds->verticalHeader()->setVisible(false);

    ui->tableWidgetHistory->setColumnCount(3);
    ui->tableWidgetHistory->setHorizontalHeaderLabels({tr("Time"), tr("Status"), tr("Note")});
    ui->tableWidgetHistory->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidgetHistory->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    ui->spinBoxMaxPrice->setMinimum(0);

    bindConnections();
    loadCategoryFilter();
    refreshQueue();
}

PendingAdsWindow::~PendingAdsWindow()
{
    delete ui;
}

void PendingAdsWindow::bindConnections()
{
    connect(ui->pushButtonRefresh, &QPushButton::clicked,
            this, &PendingAdsWindow::refreshQueue);
    connect(ui->pushButtonClearFilters, &QPushButton::clicked,
            this, &PendingAdsWindow::clearFilters);

    connect(ui->lineEditSearch, &QLineEdit::textChanged,
            this, &PendingAdsWindow::loadPendingAds);
    connect(ui->comboBoxCategoryFilter, &QComboBox::currentTextChanged,
            this, &PendingAdsWindow::loadPendingAds);
    connect(ui->checkBoxOnlyWithImage, qOverload<int>(&QCheckBox::stateChanged),
            this, [this](int) { loadPendingAds(); });
    connect(ui->spinBoxMaxPrice, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int) { loadPendingAds(); });

    connect(ui->tableViewPendingAds->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &PendingAdsWindow::onAdSelectionChanged);

    connect(ui->pushButtonApprove, &QPushButton::clicked,
            this, &PendingAdsWindow::approveSelectedAd);
    connect(ui->pushButtonReject, &QPushButton::clicked,
            this, &PendingAdsWindow::rejectSelectedAd);
    connect(ui->pushButtonSaveDraftNote, &QPushButton::clicked,
            this, &PendingAdsWindow::saveDraftNote);
}

void PendingAdsWindow::loadCategoryFilter()
{
    const QSignalBlocker blocker(ui->comboBoxCategoryFilter);

    QString currentSelection = ui->comboBoxCategoryFilter->currentText();
    ui->comboBoxCategoryFilter->clear();
    ui->comboBoxCategoryFilter->addItem(tr("All Categories"));

    AdRepository::AdListFilters filters;
    filters.sortField = AdRepository::AdListSortField::Title;
    filters.sortOrder = AdRepository::SortOrder::Asc;

    const auto allAds = adRepository_.listAdsForModeration(filters, QStringLiteral("all"), false, {}, {});
    QSet<QString> categories;
    for (const auto& ad : allAds) {
        if (!ad.category.trimmed().isEmpty()) {
            categories.insert(ad.category.trimmed());
        }
    }

    QStringList sorted = categories.values();
    sorted.sort(Qt::CaseInsensitive);
    for (const QString& category : sorted) {
        ui->comboBoxCategoryFilter->addItem(category);
    }

    const int restoredIndex = ui->comboBoxCategoryFilter->findText(currentSelection);
    if (restoredIndex >= 0) {
        ui->comboBoxCategoryFilter->setCurrentIndex(restoredIndex);
    }
}

void PendingAdsWindow::refreshQueue()
{
    loadCategoryFilter();
    loadPendingAds();
    ui->labelLastSync->setText(tr("Last refresh: %1")
                               .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
}

void PendingAdsWindow::clearFilters()
{
    ui->lineEditSearch->clear();
    ui->comboBoxCategoryFilter->setCurrentIndex(0);
    ui->checkBoxOnlyWithImage->setChecked(false);
    ui->spinBoxMaxPrice->setValue(0);
    loadPendingAds();
}

void PendingAdsWindow::loadPendingAds()
{
    AdRepository::AdListFilters filters;
    filters.nameContains = ui->lineEditSearch->text().trimmed();
    const QString category = ui->comboBoxCategoryFilter->currentText().trimmed();
    if (category.compare(tr("All Categories"), Qt::CaseInsensitive) != 0) {
        filters.category = category;
    }

    const int maxPrice = ui->spinBoxMaxPrice->value();
    if (maxPrice > 0) {
        filters.maxPriceTokens = maxPrice;
    }

    filters.sortField = AdRepository::AdListSortField::CreatedAt;
    filters.sortOrder = AdRepository::SortOrder::Desc;

    const auto ads = adRepository_.listAdsForModeration(
        filters,
        QStringLiteral("pending"),
        ui->checkBoxOnlyWithImage->isChecked(),
        {},
        ui->lineEditSearch->text().trimmed());

    populateTable(ads);
    ui->labelQueueStats->setText(tr("Pending queue: %1 ads").arg(ads.size()));

    if (ads.isEmpty()) {
        resetDetailPane();
    }
}

void PendingAdsWindow::populateTable(const QVector<AdRepository::AdSummaryRecord>& ads)
{
    auto* model = qobject_cast<QStandardItemModel*>(ui->tableViewPendingAds->model());
    if (model == nullptr) {
        return;
    }

    model->setRowCount(0);
    for (const auto& ad : ads) {
        QList<QStandardItem*> row;
        row << new QStandardItem(QString::number(ad.id));
        row << new QStandardItem(ad.title);
        row << new QStandardItem(ad.category);
        row << new QStandardItem(QString::number(ad.priceTokens));
        row << new QStandardItem(ad.sellerUsername);
        row << new QStandardItem(prettifyStatus(ad.status));
        row << new QStandardItem(ad.createdAt);
        row << new QStandardItem(ad.updatedAt);
        row << new QStandardItem(ad.hasImage ? tr("Yes") : tr("No"));
        model->appendRow(row);
    }

    if (model->rowCount() > 0) {
        ui->tableViewPendingAds->selectRow(0);
        onAdSelectionChanged();
    }
}

int PendingAdsWindow::selectedAdId() const
{
    const QModelIndex currentIndex = ui->tableViewPendingAds->currentIndex();
    if (!currentIndex.isValid()) {
        return -1;
    }

    const QModelIndex idIndex = currentIndex.siblingAtColumn(AdTableColumns::Id);
    bool ok = false;
    const int adId = idIndex.data().toInt(&ok);
    return ok ? adId : -1;
}

void PendingAdsWindow::onAdSelectionChanged()
{
    const int adId = selectedAdId();
    if (adId <= 0) {
        resetDetailPane();
        return;
    }

    loadAdDetail(adId);
}

void PendingAdsWindow::loadAdDetail(int adId)
{
    const auto ad = adRepository_.findAdById(adId);
    if (!ad.has_value()) {
        resetDetailPane();
        return;
    }

    ui->labelAdIdValue->setText(QString::number(ad->id));
    ui->labelTitleValue->setText(ad->title);
    ui->labelSellerValue->setText(ad->sellerUsername);
    ui->labelCategoryValue->setText(ad->category);
    ui->labelPriceValue->setText(tr("%1 tokens").arg(ad->priceTokens));
    ui->labelCreatedValue->setText(ad->createdAt);
    ui->labelUpdatedValue->setText(ad->updatedAt);
    ui->labelStatusValue->setText(prettifyStatus(ad->status));
    ui->labelTagsValue->setText(tr("N/A"));
    ui->textBrowserDescription->setText(ad->description);

    if (ad->imageBytes.isEmpty()) {
        ui->labelImagePreview->setText(tr("No image available"));
        ui->labelImagePreview->setPixmap(QPixmap());
        ui->labelImagePath->setText(tr("Path: [stored in database]"));
    } else {
        QPixmap preview;
        preview.loadFromData(ad->imageBytes);
        ui->labelImagePreview->setPixmap(preview.scaled(ui->labelImagePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->labelImagePath->setText(tr("Path: [binary image in database]"));
    }

    const auto history = adRepository_.getStatusHistory(adId);
    ui->tableWidgetHistory->setRowCount(history.size());
    for (int i = 0; i < history.size(); ++i) {
        const auto& item = history.at(i);
        ui->tableWidgetHistory->setItem(i, 0, new QTableWidgetItem(item.changedAt));
        ui->tableWidgetHistory->setItem(i, 1, new QTableWidgetItem(prettifyStatus(item.newStatus)));
        ui->tableWidgetHistory->setItem(i, 2, new QTableWidgetItem(item.reason));
    }
}

void PendingAdsWindow::resetDetailPane()
{
    ui->labelAdIdValue->setText(QStringLiteral("-"));
    ui->labelTitleValue->setText(QStringLiteral("-"));
    ui->labelSellerValue->setText(QStringLiteral("-"));
    ui->labelCategoryValue->setText(QStringLiteral("-"));
    ui->labelPriceValue->setText(QStringLiteral("-"));
    ui->labelCreatedValue->setText(QStringLiteral("-"));
    ui->labelUpdatedValue->setText(QStringLiteral("-"));
    ui->labelStatusValue->setText(QStringLiteral("-"));
    ui->labelTagsValue->setText(QStringLiteral("-"));
    ui->textBrowserDescription->clear();
    ui->labelImagePreview->setPixmap(QPixmap());
    ui->labelImagePreview->setText(tr("No image selected"));
    ui->labelImagePath->setText(tr("Path: -"));
    ui->tableWidgetHistory->setRowCount(0);
}

QString PendingAdsWindow::sanitizeReason() const
{
    return ui->plainTextEditRejectionReason->toPlainText().trimmed();
}

void PendingAdsWindow::approveSelectedAd()
{
    applyStatusUpdate(QStringLiteral("approve"));
}

void PendingAdsWindow::rejectSelectedAd()
{
    applyStatusUpdate(QStringLiteral("reject"));
}

void PendingAdsWindow::applyStatusUpdate(const QString& action)
{
    const int adId = selectedAdId();
    if (adId <= 0) {
        QMessageBox::warning(this, tr("Moderation"), tr("Please select an ad first."));
        return;
    }

    const AdRepository::AdModerationStatus status = parseAction(action);
    if (status == AdRepository::AdModerationStatus::Unknown) {
        return;
    }

    const QString reason = sanitizeReason();
    if (status == AdRepository::AdModerationStatus::Rejected && reason.isEmpty()) {
        QMessageBox::warning(this, tr("Moderation"), tr("Rejection reason is required."));
        return;
    }

    const bool updated = adRepository_.updateStatus(
        adId,
        status,
        reason.isEmpty() ? tr("approved by server moderator") : reason);

    if (!updated) {
        QMessageBox::critical(this, tr("Moderation"), tr("Failed to update ad status."));
        return;
    }

    ui->plainTextEditRejectionReason->clear();
    refreshQueue();
    ui->statusbar->showMessage(tr("Ad #%1 updated successfully.").arg(adId), 3000);
}

void PendingAdsWindow::saveDraftNote()
{
    const QString note = sanitizeReason();
    if (note.isEmpty()) {
        ui->statusbar->showMessage(tr("Draft note is empty."), 2000);
        return;
    }

    ui->statusbar->showMessage(tr("Draft note kept locally."), 2000);
}
