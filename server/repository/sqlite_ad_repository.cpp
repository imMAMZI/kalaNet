#include "sqlite_ad_repository.h"

#include <QCoreApplication>
#include <QDir>
#include <QMutexLocker>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <stdexcept>

namespace {

struct Migration { int version; const char* statement; };

constexpr Migration kMigrations[] = {
    {
        1,
        "CREATE TABLE IF NOT EXISTS ads ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    title TEXT NOT NULL,"
        "    description TEXT NOT NULL,"
        "    category TEXT NOT NULL,"
        "    price_tokens INTEGER NOT NULL CHECK (price_tokens > 0),"
        "    seller_username TEXT NOT NULL,"
        "    image_bytes BLOB,"
        "    status TEXT NOT NULL,"
        "    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");"
    },
    {
        2,
        "CREATE TABLE IF NOT EXISTS ad_status_history ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    ad_id INTEGER NOT NULL,"
        "    previous_status TEXT,"
        "    new_status TEXT NOT NULL,"
        "    changed_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "    reason TEXT,"
        "    FOREIGN KEY(ad_id) REFERENCES ads(id) ON DELETE CASCADE"
        ");"
    },
    {
        3,
        "CREATE INDEX IF NOT EXISTS idx_ads_status_created_at "
        "ON ads(status, created_at DESC);"
    },
    {
        4,
        "CREATE INDEX IF NOT EXISTS idx_ads_seller_username "
        "ON ads(seller_username);"
    },
    {
        5,
        "CREATE INDEX IF NOT EXISTS idx_ad_status_history_ad_id_changed_at "
        "ON ad_status_history(ad_id, changed_at DESC);"
    }
};

constexpr int kLatestSchemaVersion = sizeof(kMigrations) / sizeof(kMigrations[0]);


QString sortFieldToSqlColumn(AdRepository::AdListSortField field)
{
    switch (field) {
    case AdRepository::AdListSortField::Title:
        return QStringLiteral("title");
    case AdRepository::AdListSortField::PriceTokens:
        return QStringLiteral("price_tokens");
    case AdRepository::AdListSortField::CreatedAt:
    default:
        return QStringLiteral("created_at");
    }
}

QString moderationStatusToDb(AdRepository::AdModerationStatus status)
{
    switch (status) {
    case AdRepository::AdModerationStatus::Pending:
        return QStringLiteral("pending");
    case AdRepository::AdModerationStatus::Approved:
        return QStringLiteral("approved");
    case AdRepository::AdModerationStatus::Rejected:
        return QStringLiteral("rejected");
    case AdRepository::AdModerationStatus::Sold:
        return QStringLiteral("sold");
    case AdRepository::AdModerationStatus::Unknown:
    default:
        return QString();
    }
}

}

SqliteAdRepository::SqliteAdRepository(const QString& databasePath)
{
    databasePath_ = databasePath;
    if (databasePath_.isEmpty()) {
        databasePath_ = QCoreApplication::applicationDirPath()
                        + QDir::separator()
                        + QStringLiteral("kalanet.db");
    }

    connectionName_ = QStringLiteral("kalanet_ads_repo_%1")
                          .arg(reinterpret_cast<quintptr>(this));

    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                    connectionName_);
    db_.setDatabaseName(databasePath_);

    if (!db_.open()) {
        throwDatabaseError(QStringLiteral("open database"), db_.lastError());
    }

    initializeSchema();
}

SqliteAdRepository::~SqliteAdRepository()
{
    if (db_.isValid()) {
        db_.close();
    }
    QSqlDatabase::removeDatabase(connectionName_);
}

bool SqliteAdRepository::ensureConnection()
{
    if (db_.isOpen())
        return true;

    if (!db_.open()) {
        throwDatabaseError(QStringLiteral("reopen database"), db_.lastError());
    }
    return true;
}

void SqliteAdRepository::initializeSchema()
{
    QMutexLocker locker(&mutex_);

    ensureConnection();

    QSqlQuery pragma(db_);
    if (!pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON;"))) {
        throwDatabaseError(QStringLiteral("enable foreign keys"), pragma.lastError());
    }

    QSqlQuery migrationTable(db_);
    if (!migrationTable.exec(
            QStringLiteral("CREATE TABLE IF NOT EXISTS schema_migrations ("
                           "    version INTEGER PRIMARY KEY,"
                           "    applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                           ");"))) {
        throwDatabaseError(QStringLiteral("create schema_migrations table"),
                           migrationTable.lastError());
    }

    const int currentVersion = currentSchemaVersion();

    for (const auto& migration : kMigrations) {
        if (migration.version > currentVersion) {
            applyMigration(migration.version, migration.statement);
        }
    }

    if (currentSchemaVersion() != kLatestSchemaVersion) {
        throw std::runtime_error("Ad schema migration did not reach latest version");
    }
}

int SqliteAdRepository::currentSchemaVersion()
{
    QSqlQuery query(db_);
    if (!query.exec(QStringLiteral("SELECT COALESCE(MAX(version), 0) FROM schema_migrations;"))) {
        throwDatabaseError(QStringLiteral("read schema version"), query.lastError());
    }

    if (!query.next()) {
        return 0;
    }

    return query.value(0).toInt();
}

void SqliteAdRepository::applyMigration(int version, const char* statement)
{
    if (!db_.transaction()) {
        throwDatabaseError(QStringLiteral("begin migration transaction"), db_.lastError());
    }

    QSqlQuery query(db_);
    if (!query.exec(QString::fromUtf8(statement))) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("apply migration %1").arg(version),
                           query.lastError());
    }

    QSqlQuery insertMigration(db_);
    insertMigration.prepare(QStringLiteral(
        "INSERT INTO schema_migrations (version) VALUES (:version);"));
    insertMigration.bindValue(QStringLiteral(":version"), version);

    if (!insertMigration.exec()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("record migration %1").arg(version),
                           insertMigration.lastError());
    }

    if (!db_.commit()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("commit migration %1").arg(version),
                           db_.lastError());
    }
}

int SqliteAdRepository::createPendingAd(const NewAd& ad)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    if (!db_.transaction()) {
        throwDatabaseError(QStringLiteral("begin createPendingAd transaction"),
                           db_.lastError());
    }

    QSqlQuery insertAd(db_);
    insertAd.prepare(QStringLiteral(
        "INSERT INTO ads ("
        "    title, description, category, price_tokens,"
        "    seller_username, image_bytes, status"
        ") VALUES ("
        "    :title, :description, :category, :price_tokens,"
        "    :seller_username, :image_bytes, :status"
        ");"));

    insertAd.bindValue(QStringLiteral(":title"), ad.title);
    insertAd.bindValue(QStringLiteral(":description"), ad.description);
    insertAd.bindValue(QStringLiteral(":category"), ad.category);
    insertAd.bindValue(QStringLiteral(":price_tokens"), ad.priceTokens);
    insertAd.bindValue(QStringLiteral(":seller_username"), ad.sellerUsername);
    insertAd.bindValue(QStringLiteral(":image_bytes"), ad.imageBytes);
    insertAd.bindValue(QStringLiteral(":status"), QStringLiteral("pending"));

    if (!insertAd.exec()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("insert ad"), insertAd.lastError());
    }

    const QVariant adIdVariant = insertAd.lastInsertId();
    if (!adIdVariant.isValid()) {
        db_.rollback();
        throw std::runtime_error("Failed to resolve inserted ad ID");
    }

    const int adId = adIdVariant.toInt();

    QSqlQuery insertHistory(db_);
    insertHistory.prepare(QStringLiteral(
        "INSERT INTO ad_status_history (ad_id, previous_status, new_status, reason) "
        "VALUES (:ad_id, NULL, :new_status, :reason);"));
    insertHistory.bindValue(QStringLiteral(":ad_id"), adId);
    insertHistory.bindValue(QStringLiteral(":new_status"), QStringLiteral("pending"));
    insertHistory.bindValue(QStringLiteral(":reason"), QStringLiteral("ad created"));

    if (!insertHistory.exec()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("insert ad status history"),
                           insertHistory.lastError());
    }

    if (!db_.commit()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("commit createPendingAd transaction"),
                           db_.lastError());
    }

    return adId;
}


QVector<AdRepository::AdSummaryRecord> SqliteAdRepository::listApprovedAds(
    const AdListFilters& filters)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QString sql = QStringLiteral(
        "SELECT id, title, category, price_tokens, seller_username, status, created_at, updated_at, image_bytes "
        "FROM ads WHERE status = :status");

    if (!filters.nameContains.trimmed().isEmpty()) {
        sql += QStringLiteral(" AND title LIKE :title");
    }

    if (!filters.category.trimmed().isEmpty()) {
        sql += QStringLiteral(" AND LOWER(category) = LOWER(:category)");
    }

    if (filters.minPriceTokens > 0) {
        sql += QStringLiteral(" AND price_tokens >= :min_price");
    }

    if (filters.maxPriceTokens > 0) {
        sql += QStringLiteral(" AND price_tokens <= :max_price");
    }

    const QString sortField = sortFieldToSqlColumn(filters.sortField);
    const QString sortOrder = filters.sortOrder == AdRepository::SortOrder::Asc
                                  ? QStringLiteral("ASC")
                                  : QStringLiteral("DESC");

    sql += QStringLiteral(" ORDER BY %1 %2, id %2;").arg(sortField, sortOrder);

    QSqlQuery query(db_);
    query.prepare(sql);
    query.bindValue(QStringLiteral(":status"), QStringLiteral("approved"));

    if (!filters.nameContains.trimmed().isEmpty()) {
        query.bindValue(QStringLiteral(":title"), QStringLiteral("%") + filters.nameContains.trimmed() + QStringLiteral("%"));
    }

    if (!filters.category.trimmed().isEmpty()) {
        query.bindValue(QStringLiteral(":category"), filters.category.trimmed());
    }

    if (filters.minPriceTokens > 0) {
        query.bindValue(QStringLiteral(":min_price"), filters.minPriceTokens);
    }

    if (filters.maxPriceTokens > 0) {
        query.bindValue(QStringLiteral(":max_price"), filters.maxPriceTokens);
    }

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("list approved ads"), query.lastError());
    }

    QVector<AdSummaryRecord> ads;
    while (query.next()) {
        AdSummaryRecord record;
        record.id = query.value(0).toInt();
        record.title = query.value(1).toString();
        record.category = query.value(2).toString();
        record.priceTokens = query.value(3).toInt();
        record.sellerUsername = query.value(4).toString();
        record.status = query.value(5).toString();
        record.createdAt = query.value(6).toString();
        record.updatedAt = query.value(7).toString();
        record.hasImage = !query.value(8).toByteArray().isEmpty();
        ads.push_back(record);
    }

    return ads;
}

std::optional<AdRepository::AdDetailRecord> SqliteAdRepository::findApprovedAdById(int adId)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT id, title, description, category, price_tokens, seller_username, image_bytes, status, created_at, updated_at "
        "FROM ads WHERE id = :id AND status = :status LIMIT 1;"));
    query.bindValue(QStringLiteral(":id"), adId);
    query.bindValue(QStringLiteral(":status"), QStringLiteral("approved"));

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("find approved ad by id"), query.lastError());
    }

    if (!query.next()) {
        return std::nullopt;
    }

    AdDetailRecord record;
    record.id = query.value(0).toInt();
    record.title = query.value(1).toString();
    record.description = query.value(2).toString();
    record.category = query.value(3).toString();
    record.priceTokens = query.value(4).toInt();
    record.sellerUsername = query.value(5).toString();
    record.imageBytes = query.value(6).toByteArray();
    record.status = query.value(7).toString();
    record.createdAt = query.value(8).toString();
    record.updatedAt = query.value(9).toString();
    return record;
}




std::optional<AdRepository::AdDetailRecord> SqliteAdRepository::findAdById(int adId)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT id, title, description, category, price_tokens, seller_username, image_bytes, status, created_at, updated_at "
        "FROM ads WHERE id = :id LIMIT 1;"));
    query.bindValue(QStringLiteral(":id"), adId);

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("find ad by id"), query.lastError());
    }

    if (!query.next()) {
        return std::nullopt;
    }

    AdDetailRecord record;
    record.id = query.value(0).toInt();
    record.title = query.value(1).toString();
    record.description = query.value(2).toString();
    record.category = query.value(3).toString();
    record.priceTokens = query.value(4).toInt();
    record.sellerUsername = query.value(5).toString();
    record.imageBytes = query.value(6).toByteArray();
    record.status = query.value(7).toString();
    record.createdAt = query.value(8).toString();
    record.updatedAt = query.value(9).toString();
    return record;
}

bool SqliteAdRepository::hasDuplicateActiveAdForSeller(const NewAd& ad)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT COUNT(1) "
        "FROM ads "
        "WHERE LOWER(title) = LOWER(:title) "
        "  AND LOWER(category) = LOWER(:category) "
        "  AND price_tokens = :price_tokens "
        "  AND seller_username = :seller_username "
        "  AND status IN ('pending', 'approved', 'sold')"));

    query.bindValue(QStringLiteral(":title"), ad.title.trimmed());
    query.bindValue(QStringLiteral(":category"), ad.category.trimmed());
    query.bindValue(QStringLiteral(":price_tokens"), ad.priceTokens);
    query.bindValue(QStringLiteral(":seller_username"), ad.sellerUsername.trimmed());

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("detect duplicate ad"), query.lastError());
    }

    if (!query.next()) {
        return false;
    }

    return query.value(0).toInt() > 0;
}


QVector<AdRepository::AdSummaryRecord> SqliteAdRepository::listAdsForModeration(
    const AdListFilters& filters,
    const QString& statusFilter,
    bool onlyWithImage,
    const QString& sellerContains,
    const QString& fullTextContains)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QString sql = QStringLiteral(
        "SELECT id, title, category, price_tokens, seller_username, status, created_at, updated_at, image_bytes "
        "FROM ads WHERE 1 = 1");

    if (!statusFilter.trimmed().isEmpty() && statusFilter.compare(QStringLiteral("all"), Qt::CaseInsensitive) != 0) {
        sql += QStringLiteral(" AND status = :status");
    }
    if (!filters.nameContains.trimmed().isEmpty()) {
        sql += QStringLiteral(" AND title LIKE :title");
    }
    if (!filters.category.trimmed().isEmpty()) {
        sql += QStringLiteral(" AND LOWER(category) = LOWER(:category)");
    }
    if (filters.minPriceTokens > 0) {
        sql += QStringLiteral(" AND price_tokens >= :min_price");
    }
    if (filters.maxPriceTokens > 0) {
        sql += QStringLiteral(" AND price_tokens <= :max_price");
    }
    if (onlyWithImage) {
        sql += QStringLiteral(" AND image_bytes IS NOT NULL AND length(image_bytes) > 0");
    }
    if (!sellerContains.trimmed().isEmpty()) {
        sql += QStringLiteral(" AND seller_username LIKE :seller");
    }
    if (!fullTextContains.trimmed().isEmpty()) {
        sql += QStringLiteral(" AND (title LIKE :text_query OR description LIKE :text_query)");
    }

    const QString sortField = sortFieldToSqlColumn(filters.sortField);
    const QString sortOrder = filters.sortOrder == AdRepository::SortOrder::Asc
                                  ? QStringLiteral("ASC")
                                  : QStringLiteral("DESC");
    sql += QStringLiteral(" ORDER BY %1 %2, id %2;").arg(sortField, sortOrder);

    QSqlQuery query(db_);
    query.prepare(sql);

    if (!statusFilter.trimmed().isEmpty() && statusFilter.compare(QStringLiteral("all"), Qt::CaseInsensitive) != 0) {
        query.bindValue(QStringLiteral(":status"), statusFilter.trimmed().toLower());
    }
    if (!filters.nameContains.trimmed().isEmpty()) {
        query.bindValue(QStringLiteral(":title"), QStringLiteral("%") + filters.nameContains.trimmed() + QStringLiteral("%"));
    }
    if (!filters.category.trimmed().isEmpty()) {
        query.bindValue(QStringLiteral(":category"), filters.category.trimmed());
    }
    if (filters.minPriceTokens > 0) {
        query.bindValue(QStringLiteral(":min_price"), filters.minPriceTokens);
    }
    if (filters.maxPriceTokens > 0) {
        query.bindValue(QStringLiteral(":max_price"), filters.maxPriceTokens);
    }
    if (!sellerContains.trimmed().isEmpty()) {
        query.bindValue(QStringLiteral(":seller"), QStringLiteral("%") + sellerContains.trimmed() + QStringLiteral("%"));
    }
    if (!fullTextContains.trimmed().isEmpty()) {
        query.bindValue(QStringLiteral(":text_query"), QStringLiteral("%") + fullTextContains.trimmed() + QStringLiteral("%"));
    }

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("list ads for moderation"), query.lastError());
    }

    QVector<AdSummaryRecord> ads;
    while (query.next()) {
        AdSummaryRecord record;
        record.id = query.value(0).toInt();
        record.title = query.value(1).toString();
        record.category = query.value(2).toString();
        record.priceTokens = query.value(3).toInt();
        record.sellerUsername = query.value(4).toString();
        record.status = query.value(5).toString();
        record.createdAt = query.value(6).toString();
        record.updatedAt = query.value(7).toString();
        record.hasImage = !query.value(8).toByteArray().isEmpty();
        ads.push_back(record);
    }

    return ads;
}

QVector<AdRepository::AdStatusHistoryRecord> SqliteAdRepository::getStatusHistory(int adId)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT previous_status, new_status, reason, changed_at "
        "FROM ad_status_history WHERE ad_id = :ad_id ORDER BY changed_at DESC, id DESC;"));
    query.bindValue(QStringLiteral(":ad_id"), adId);

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("get ad status history"), query.lastError());
    }

    QVector<AdStatusHistoryRecord> history;
    while (query.next()) {
        AdStatusHistoryRecord item;
        item.previousStatus = query.value(0).toString();
        item.newStatus = query.value(1).toString();
        item.reason = query.value(2).toString();
        item.changedAt = query.value(3).toString();
        history.push_back(item);
    }

    return history;
}

QVector<AdRepository::AdTransactionHistoryRecord> SqliteAdRepository::getTransactionHistoryForAd(int adId,
                                                                                                    int limit)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT created_at, type, username, counterparty, amount_tokens, balance_after "
        "FROM transaction_ledger "
        "WHERE ad_id = :ad_id "
        "ORDER BY created_at DESC, id DESC "
        "LIMIT :limit;"));
    query.bindValue(QStringLiteral(":ad_id"), adId);
    query.bindValue(QStringLiteral(":limit"), limit > 0 ? limit : 100);

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("get ad transaction history"), query.lastError());
    }

    QVector<AdTransactionHistoryRecord> records;
    while (query.next()) {
        AdTransactionHistoryRecord record;
        record.createdAt = query.value(0).toString();
        record.entryType = query.value(1).toString();
        record.username = query.value(2).toString();
        record.counterparty = query.value(3).toString();
        record.amountTokens = query.value(4).toInt();
        record.balanceAfter = query.value(5).toInt();
        records.push_back(record);
    }

    return records;
}


bool SqliteAdRepository::updateStatus(int adId,
                                      AdModerationStatus newStatus,
                                      const QString& reason)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    const QString newStatusDb = moderationStatusToDb(newStatus);
    if (newStatusDb.isEmpty()) {
        return false;
    }

    if (!db_.transaction()) {
        throwDatabaseError(QStringLiteral("begin update ad status transaction"), db_.lastError());
    }

    QSqlQuery currentStatusQuery(db_);
    currentStatusQuery.prepare(QStringLiteral("SELECT status FROM ads WHERE id = :id LIMIT 1;"));
    currentStatusQuery.bindValue(QStringLiteral(":id"), adId);

    if (!currentStatusQuery.exec()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("read current ad status"), currentStatusQuery.lastError());
    }

    if (!currentStatusQuery.next()) {
        db_.rollback();
        return false;
    }

    const QString previousStatus = currentStatusQuery.value(0).toString();

    QSqlQuery updateQuery(db_);
    updateQuery.prepare(QStringLiteral(
        "UPDATE ads SET status = :new_status, updated_at = CURRENT_TIMESTAMP "
        "WHERE id = :id;"));
    updateQuery.bindValue(QStringLiteral(":new_status"), newStatusDb);
    updateQuery.bindValue(QStringLiteral(":id"), adId);

    if (!updateQuery.exec()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("update ad status"), updateQuery.lastError());
    }

    if (updateQuery.numRowsAffected() <= 0) {
        db_.rollback();
        return false;
    }

    QSqlQuery insertHistory(db_);
    insertHistory.prepare(QStringLiteral(
        "INSERT INTO ad_status_history (ad_id, previous_status, new_status, reason) "
        "VALUES (:ad_id, :previous_status, :new_status, :reason);"));
    insertHistory.bindValue(QStringLiteral(":ad_id"), adId);
    insertHistory.bindValue(QStringLiteral(":previous_status"), previousStatus);
    insertHistory.bindValue(QStringLiteral(":new_status"), newStatusDb);
    insertHistory.bindValue(QStringLiteral(":reason"), reason.trimmed());

    if (!insertHistory.exec()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("insert ad status update history"), insertHistory.lastError());
    }

    if (!db_.commit()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("commit ad status update transaction"), db_.lastError());
    }

    return true;
}

[[noreturn]] void SqliteAdRepository::throwDatabaseError(
    const QString& context,
    const QSqlError& error) const
{
    const QString message = QStringLiteral("Database error (%1): %2")
                                .arg(context, error.text());
    throw std::runtime_error(message.toStdString());
}

QVector<AdRepository::AdSummaryRecord> SqliteAdRepository::listAdsBySeller(const QString& sellerUsername,
                                                                            const QString& statusFilter)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QString sql = QStringLiteral(
        "SELECT id, title, category, price_tokens, seller_username, status, created_at, updated_at, image_bytes "
        "FROM ads WHERE seller_username = :seller_username");
    if (!statusFilter.trimmed().isEmpty()) {
        sql += QStringLiteral(" AND status = :status");
    }
    sql += QStringLiteral(" ORDER BY created_at DESC, id DESC;");

    QSqlQuery query(db_);
    query.prepare(sql);
    query.bindValue(QStringLiteral(":seller_username"), sellerUsername.trimmed());
    if (!statusFilter.trimmed().isEmpty()) {
        query.bindValue(QStringLiteral(":status"), statusFilter.trimmed().toLower());
    }

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("listAdsBySeller"), query.lastError());
    }

    QVector<AdSummaryRecord> ads;
    while (query.next()) {
        AdSummaryRecord record;
        record.id = query.value(0).toInt();
        record.title = query.value(1).toString();
        record.category = query.value(2).toString();
        record.priceTokens = query.value(3).toInt();
        record.sellerUsername = query.value(4).toString();
        record.status = query.value(5).toString();
        record.createdAt = query.value(6).toString();
        record.updatedAt = query.value(7).toString();
        record.hasImage = !query.value(8).toByteArray().isEmpty();
        ads.push_back(record);
    }

    return ads;
}

QVector<AdRepository::AdSummaryRecord> SqliteAdRepository::listPurchasedAdsByBuyer(const QString& buyerUsername,
                                                                                     int limit)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT a.id, a.title, a.category, a.price_tokens, a.seller_username, a.status, tl.created_at, a.updated_at, a.image_bytes "
        "FROM transaction_ledger tl "
        "JOIN ads a ON a.id = tl.ad_id "
        "WHERE tl.username = :username AND tl.type = 'purchase_debit' "
        "ORDER BY tl.created_at DESC, tl.id DESC LIMIT :limit;"));
    query.bindValue(QStringLiteral(":username"), buyerUsername.trimmed());
    query.bindValue(QStringLiteral(":limit"), limit > 0 ? limit : 100);

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("listPurchasedAdsByBuyer"), query.lastError());
    }

    QVector<AdSummaryRecord> ads;
    while (query.next()) {
        AdSummaryRecord record;
        record.id = query.value(0).toInt();
        record.title = query.value(1).toString();
        record.category = query.value(2).toString();
        record.priceTokens = query.value(3).toInt();
        record.sellerUsername = query.value(4).toString();
        record.status = query.value(5).toString();
        record.createdAt = query.value(6).toString();
        record.updatedAt = query.value(7).toString();
        record.hasImage = !query.value(8).toByteArray().isEmpty();
        ads.push_back(record);
    }

    return ads;
}

AdRepository::AdStatusCounts SqliteAdRepository::getAdStatusCounts()
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    AdStatusCounts counts;
    QSqlQuery query(db_);
    if (!query.exec(QStringLiteral(
            "SELECT status, COUNT(1) FROM ads GROUP BY status;"))) {
        throwDatabaseError(QStringLiteral("getAdStatusCounts"), query.lastError());
    }

    while (query.next()) {
        const QString status = query.value(0).toString().trimmed().toLower();
        const int count = query.value(1).toInt();
        if (status == QStringLiteral("pending")) counts.pending = count;
        if (status == QStringLiteral("approved")) counts.approved = count;
        if (status == QStringLiteral("sold")) counts.sold = count;
    }

    return counts;
}

AdRepository::SalesTotals SqliteAdRepository::getSalesTotals()
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    SalesTotals totals;
    QSqlQuery query(db_);
    if (!query.exec(QStringLiteral(
            "SELECT COUNT(1), COALESCE(SUM(price_tokens), 0) FROM ads WHERE status = 'sold';"))) {
        throwDatabaseError(QStringLiteral("getSalesTotals"), query.lastError());
    }

    if (query.next()) {
        totals.soldAdsCount = query.value(0).toInt();
        totals.totalTokens = query.value(1).toInt();
    }

    return totals;
}
