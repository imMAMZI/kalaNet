#ifndef KALANET_PENDINGADSWINDOW_H
#define KALANET_PENDINGADSWINDOW_H

#include <QMainWindow>
#include <QVector>

#include "../repository/ad_repository.h"

namespace AdTableColumns {
enum Value {
    Id = 0,
    Title,
    Category,
    Price,
    Seller,
    Status,
    CreatedAt,
    UpdatedAt,
    HasImage,
    Count
};
}

QT_BEGIN_NAMESPACE
namespace Ui { class PendingAdsWindow; }
QT_END_NAMESPACE

class PendingAdsWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PendingAdsWindow(AdRepository& adRepository, QWidget* parent = nullptr);
    ~PendingAdsWindow() override;

private slots:
    void refreshQueue();
    void clearFilters();
    void onAdSelectionChanged();
    void approveSelectedAd();
    void rejectSelectedAd();
    void saveDraftNote();

private:
    void bindConnections();
    void loadCategoryFilter();
    void loadPendingAds();
    void populateTable(const QVector<AdRepository::AdSummaryRecord>& ads);
    int selectedAdId() const;
    void loadAdDetail(int adId);
    void resetDetailPane();
    void applyStatusUpdate(const QString& action);
    QString sanitizeReason() const;

    AdRepository& adRepository_;
    Ui::PendingAdsWindow* ui;
};

#endif // KALANET_PENDINGADSWINDOW_H
