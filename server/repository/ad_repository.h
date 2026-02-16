#ifndef AD_REPOSITORY_H
#define AD_REPOSITORY_H

#include <QByteArray>
#include <QString>
#include <QVector>

#include <optional>

class AdRepository
{
public:
    enum class AdListSortField {
        CreatedAt,
        Title,
        PriceTokens
    };

    enum class SortOrder {
        Desc,
        Asc
    };

    enum class AdModerationStatus {
        Pending,
        Approved,
        Rejected,
        Sold,
        Unknown
    };

    struct AdListFilters {
        QString nameContains;
        QString category;
        int minPriceTokens = 0;
        int maxPriceTokens = 0;
        AdListSortField sortField = AdListSortField::CreatedAt;
        SortOrder sortOrder = SortOrder::Desc;
    };

    struct NewAd {
        QString title;
        QString description;
        QString category;
        int priceTokens = 0;
        QString sellerUsername;
        QByteArray imageBytes;
    };

    struct AdSummaryRecord {
        int id = -1;
        QString title;
        QString category;
        int priceTokens = 0;
        QString sellerUsername;
        QString status;
        QString createdAt;
        QString updatedAt;
        bool hasImage = false;
    };

    struct AdStatusHistoryRecord {
        QString previousStatus;
        QString newStatus;
        QString reason;
        QString changedAt;
    };

    struct AdTransactionHistoryRecord {
        QString createdAt;
        QString entryType;
        QString username;
        QString counterparty;
        int amountTokens = 0;
        int balanceAfter = 0;
    };

    struct AdStatusCounts {
        int pending = 0;
        int approved = 0;
        int sold = 0;
    };

    struct SalesTotals {
        int soldAdsCount = 0;
        int totalTokens = 0;
    };

    struct AdDetailRecord {
        int id = -1;
        QString title;
        QString description;
        QString category;
        int priceTokens = 0;
        QString sellerUsername;
        QByteArray imageBytes;
        QString status;
        QString createdAt;
        QString updatedAt;
    };

    virtual ~AdRepository() = default;

    virtual int createPendingAd(const NewAd& ad) = 0;
    virtual QVector<AdSummaryRecord> listApprovedAds(const AdListFilters& filters) = 0;
    virtual std::optional<AdDetailRecord> findApprovedAdById(int adId) = 0;
    virtual std::optional<AdDetailRecord> findAdById(int adId) = 0;
    virtual bool hasDuplicateActiveAdForSeller(const NewAd& ad) = 0;
    virtual bool updateStatus(int adId,
                              AdModerationStatus newStatus,
                              const QString& reason) = 0;
    virtual QVector<AdSummaryRecord> listAdsForModeration(const AdListFilters& filters,
                                                           const QString& statusFilter,
                                                           bool onlyWithImage,
                                                           const QString& sellerContains,
                                                           const QString& fullTextContains) = 0;
    virtual QVector<AdStatusHistoryRecord> getStatusHistory(int adId) = 0;
    virtual QVector<AdTransactionHistoryRecord> getTransactionHistoryForAd(int adId, int limit = 100) = 0;
    virtual QVector<AdSummaryRecord> listAdsBySeller(const QString& sellerUsername, const QString& statusFilter) = 0;
    virtual QVector<AdSummaryRecord> listPurchasedAdsByBuyer(const QString& buyerUsername, int limit) = 0;
    virtual AdStatusCounts getAdStatusCounts() = 0;
    virtual SalesTotals getSalesTotals() = 0;
};

#endif // AD_REPOSITORY_H
