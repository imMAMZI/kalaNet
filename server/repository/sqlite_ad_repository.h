#ifndef SQLITE_AD_REPOSITORY_H
#define SQLITE_AD_REPOSITORY_H

#include "ad_repository.h"

#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>

#include <optional>

class SqliteAdRepository : public AdRepository
{
public:
    explicit SqliteAdRepository(const QString& databasePath = QString());
    ~SqliteAdRepository() override;

    int createPendingAd(const NewAd& ad) override;
    QVector<AdSummaryRecord> listApprovedAds(const AdListFilters& filters) override;
    std::optional<AdDetailRecord> findApprovedAdById(int adId) override;
    std::optional<AdDetailRecord> findAdById(int adId) override;
    bool hasDuplicateActiveAdForSeller(const NewAd& ad) override;
    bool updateStatus(int adId,
                      AdModerationStatus newStatus,
                      const QString& reason) override;
    QVector<AdSummaryRecord> listAdsBySeller(const QString& sellerUsername, const QString& statusFilter) override;
    QVector<AdSummaryRecord> listPurchasedAdsByBuyer(const QString& buyerUsername, int limit) override;
    AdStatusCounts getAdStatusCounts() override;
    SalesTotals getSalesTotals() override;

private:
    bool ensureConnection();
    void initializeSchema();
    int currentSchemaVersion();
    void applyMigration(int version, const char* statement);
    [[noreturn]] void throwDatabaseError(const QString& context,
                                         const QSqlError& error) const;

    QSqlDatabase db_;
    QString connectionName_;
    QString databasePath_;
    QMutex mutex_;
};

#endif // SQLITE_AD_REPOSITORY_H
