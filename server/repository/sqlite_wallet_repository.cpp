#include "sqlite_wallet_repository.h"

#include <QCoreApplication>
#include <QDir>
#include <QMap>
#include <QMutexLocker>
#include <QSqlQuery>
#include <QVariant>

#include <stdexcept>
#include <utility>

SqliteWalletRepository::SqliteWalletRepository(const QString& databasePath)
{
    databasePath_ = databasePath;
    if (databasePath_.isEmpty()) {
        databasePath_ = QCoreApplication::applicationDirPath()
                        + QDir::separator()
                        + QStringLiteral("kalanet.db");
    }

    connectionName_ = QStringLiteral("kalanet_wallet_repo_%1")
                          .arg(reinterpret_cast<quintptr>(this));

    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    db_.setDatabaseName(databasePath_);

    if (!db_.open()) {
        throwDatabaseError(QStringLiteral("open database"), db_.lastError());
    }

    initializeSchema();
}

SqliteWalletRepository::~SqliteWalletRepository()
{
    if (db_.isValid()) {
        db_.close();
    }
    QSqlDatabase::removeDatabase(connectionName_);
}

bool SqliteWalletRepository::ensureConnection()
{
    if (db_.isOpen()) {
        return true;
    }

    if (!db_.open()) {
        throwDatabaseError(QStringLiteral("reopen database"), db_.lastError());
    }
    return true;
}

void SqliteWalletRepository::initializeSchema()
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery createWallets(db_);
    if (!createWallets.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS wallets ("
            " username TEXT PRIMARY KEY,"
            " balance_tokens INTEGER NOT NULL DEFAULT 0"
            ");"))) {
        throwDatabaseError(QStringLiteral("create wallets table"), createWallets.lastError());
    }

    QSqlQuery createLedger(db_);
    if (!createLedger.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS transaction_ledger ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " username TEXT NOT NULL,"
            " type TEXT NOT NULL,"
            " amount_tokens INTEGER NOT NULL,"
            " balance_after INTEGER NOT NULL,"
            " ad_id INTEGER,"
            " counterparty TEXT,"
            " created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ");"))) {
        throwDatabaseError(QStringLiteral("create transaction ledger table"), createLedger.lastError());
    }

    QSqlQuery createIndex(db_);
    if (!createIndex.exec(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_transaction_ledger_user_created "
            "ON transaction_ledger(username, created_at DESC);"))) {
        throwDatabaseError(QStringLiteral("create transaction ledger index"), createIndex.lastError());
    }
}

void SqliteWalletRepository::ensureWalletRow(const QString& username)
{
    QSqlQuery insert(db_);
    insert.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO wallets (username, balance_tokens) VALUES (:username, 0);"));
    insert.bindValue(QStringLiteral(":username"), username.trimmed());

    if (!insert.exec()) {
        throwDatabaseError(QStringLiteral("ensure wallet row"), insert.lastError());
    }
}

int SqliteWalletRepository::getBalance(const QString& username)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    const QString normalized = username.trimmed();
    ensureWalletRow(normalized);

    QSqlQuery select(db_);
    select.prepare(QStringLiteral(
        "SELECT balance_tokens FROM wallets WHERE username = :username LIMIT 1;"));
    select.bindValue(QStringLiteral(":username"), normalized);

    if (!select.exec()) {
        throwDatabaseError(QStringLiteral("get wallet balance"), select.lastError());
    }

    return select.next() ? select.value(0).toInt() : 0;
}

int SqliteWalletRepository::topUp(const QString& username, int amountTokens)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    const QString normalized = username.trimmed();
    ensureWalletRow(normalized);

    if (!db_.transaction()) {
        throwDatabaseError(QStringLiteral("begin top-up transaction"), db_.lastError());
    }

    QSqlQuery update(db_);
    update.prepare(QStringLiteral(
        "UPDATE wallets SET balance_tokens = balance_tokens + :amount WHERE username = :username;"));
    update.bindValue(QStringLiteral(":amount"), amountTokens);
    update.bindValue(QStringLiteral(":username"), normalized);

    if (!update.exec()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("top-up wallet"), update.lastError());
    }

    QSqlQuery balanceQuery(db_);
    balanceQuery.prepare(QStringLiteral(
        "SELECT balance_tokens FROM wallets WHERE username = :username LIMIT 1;"));
    balanceQuery.bindValue(QStringLiteral(":username"), normalized);
    if (!balanceQuery.exec() || !balanceQuery.next()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("load top-up balance"), balanceQuery.lastError());
    }
    const int newBalance = balanceQuery.value(0).toInt();

    QSqlQuery ledger(db_);
    ledger.prepare(QStringLiteral(
        "INSERT INTO transaction_ledger(username, type, amount_tokens, balance_after, ad_id, counterparty) "
        "VALUES(:username, 'topup', :amount, :balance_after, NULL, NULL);"));
    ledger.bindValue(QStringLiteral(":username"), normalized);
    ledger.bindValue(QStringLiteral(":amount"), amountTokens);
    ledger.bindValue(QStringLiteral(":balance_after"), newBalance);

    if (!ledger.exec()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("insert top-up ledger"), ledger.lastError());
    }

    if (!db_.commit()) {
        throwDatabaseError(QStringLiteral("commit top-up transaction"), db_.lastError());
    }

    return newBalance;
}

bool SqliteWalletRepository::checkout(const QString& buyerUsername,
                                      const QVector<int>& adIds,
                                      CheckoutResult& result,
                                      QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    result = CheckoutResult{};
    const QString buyer = buyerUsername.trimmed();
    ensureWalletRow(buyer);

    if (!db_.transaction()) {
        throwDatabaseError(QStringLiteral("begin checkout transaction"), db_.lastError());
    }

    QMap<QString, int> sellerCredits;
    int totalCost = 0;

    for (const int adId : adIds) {
        QSqlQuery adQuery(db_);
        adQuery.prepare(QStringLiteral(
            "SELECT seller_username, price_tokens, status FROM ads WHERE id = :id LIMIT 1;"));
        adQuery.bindValue(QStringLiteral(":id"), adId);

        if (!adQuery.exec()) {
            db_.rollback();
            throwDatabaseError(QStringLiteral("load ad for checkout"), adQuery.lastError());
        }

        if (!adQuery.next()) {
            db_.rollback();
            if (errorMessage) {
                *errorMessage = QStringLiteral("Advertisement %1 not found").arg(adId);
            }
            return false;
        }

        const QString seller = adQuery.value(0).toString().trimmed();
        const int price = adQuery.value(1).toInt();
        const QString status = adQuery.value(2).toString().trimmed().toLower();

        if (status != QStringLiteral("approved")) {
            db_.rollback();
            if (errorMessage) {
                *errorMessage = QStringLiteral("Advertisement %1 is not available").arg(adId);
            }
            return false;
        }

        if (seller.compare(buyer, Qt::CaseInsensitive) == 0) {
            db_.rollback();
            if (errorMessage) {
                *errorMessage = QStringLiteral("Cannot buy your own advertisement");
            }
            return false;
        }

        totalCost += price;
        sellerCredits[seller] += price;

        CheckoutItem item;
        item.adId = adId;
        item.priceTokens = price;
        item.sellerUsername = seller;
        result.purchasedItems.push_back(item);
    }

    QSqlQuery buyerBalanceQuery(db_);
    buyerBalanceQuery.prepare(QStringLiteral(
        "SELECT balance_tokens FROM wallets WHERE username = :username LIMIT 1;"));
    buyerBalanceQuery.bindValue(QStringLiteral(":username"), buyer);

    if (!buyerBalanceQuery.exec() || !buyerBalanceQuery.next()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("load buyer balance"), buyerBalanceQuery.lastError());
    }

    const int buyerBalance = buyerBalanceQuery.value(0).toInt();
    if (buyerBalance < totalCost) {
        db_.rollback();
        if (errorMessage) {
            *errorMessage = QStringLiteral("Insufficient wallet balance");
        }
        return false;
    }

    QSqlQuery debit(db_);
    debit.prepare(QStringLiteral(
        "UPDATE wallets SET balance_tokens = balance_tokens - :amount WHERE username = :username;"));
    debit.bindValue(QStringLiteral(":amount"), totalCost);
    debit.bindValue(QStringLiteral(":username"), buyer);
    if (!debit.exec()) {
        db_.rollback();
        throwDatabaseError(QStringLiteral("debit buyer"), debit.lastError());
    }

    result.buyerBalance = buyerBalance - totalCost;

    for (auto it = sellerCredits.cbegin(); it != sellerCredits.cend(); ++it) {
        ensureWalletRow(it.key());

        QSqlQuery credit(db_);
        credit.prepare(QStringLiteral(
            "UPDATE wallets SET balance_tokens = balance_tokens + :amount WHERE username = :username;"));
        credit.bindValue(QStringLiteral(":amount"), it.value());
        credit.bindValue(QStringLiteral(":username"), it.key());
        if (!credit.exec()) {
            db_.rollback();
            throwDatabaseError(QStringLiteral("credit seller"), credit.lastError());
        }
    }

    for (const CheckoutItem& item : std::as_const(result.purchasedItems)) {
        QSqlQuery markSold(db_);
        markSold.prepare(QStringLiteral(
            "UPDATE ads SET status = 'sold', updated_at = CURRENT_TIMESTAMP "
            "WHERE id = :id AND status = 'approved';"));
        markSold.bindValue(QStringLiteral(":id"), item.adId);
        if (!markSold.exec() || markSold.numRowsAffected() != 1) {
            db_.rollback();
            if (errorMessage) {
                *errorMessage = QStringLiteral("Advertisement %1 is no longer available").arg(item.adId);
            }
            return false;
        }

        QSqlQuery buyerLedger(db_);
        buyerLedger.prepare(QStringLiteral(
            "INSERT INTO transaction_ledger(username, type, amount_tokens, balance_after, ad_id, counterparty) "
            "VALUES(:username, 'purchase_debit', :amount, "
            "(SELECT balance_tokens FROM wallets WHERE username = :username), :ad_id, :counterparty);"));
        buyerLedger.bindValue(QStringLiteral(":username"), buyer);
        buyerLedger.bindValue(QStringLiteral(":amount"), -item.priceTokens);
        buyerLedger.bindValue(QStringLiteral(":ad_id"), item.adId);
        buyerLedger.bindValue(QStringLiteral(":counterparty"), item.sellerUsername);
        if (!buyerLedger.exec()) {
            db_.rollback();
            throwDatabaseError(QStringLiteral("insert buyer ledger"), buyerLedger.lastError());
        }

        QSqlQuery sellerLedger(db_);
        sellerLedger.prepare(QStringLiteral(
            "INSERT INTO transaction_ledger(username, type, amount_tokens, balance_after, ad_id, counterparty) "
            "VALUES(:username, 'sale_credit', :amount, "
            "(SELECT balance_tokens FROM wallets WHERE username = :username), :ad_id, :counterparty);"));
        sellerLedger.bindValue(QStringLiteral(":username"), item.sellerUsername);
        sellerLedger.bindValue(QStringLiteral(":amount"), item.priceTokens);
        sellerLedger.bindValue(QStringLiteral(":ad_id"), item.adId);
        sellerLedger.bindValue(QStringLiteral(":counterparty"), buyer);
        if (!sellerLedger.exec()) {
            db_.rollback();
            throwDatabaseError(QStringLiteral("insert seller ledger"), sellerLedger.lastError());
        }

        QSqlQuery cartCleanup(db_);
        cartCleanup.prepare(QStringLiteral(
            "DELETE FROM cart_items WHERE username = :username AND ad_id = :ad_id;"));
        cartCleanup.bindValue(QStringLiteral(":username"), buyer);
        cartCleanup.bindValue(QStringLiteral(":ad_id"), item.adId);
        if (!cartCleanup.exec()) {
            db_.rollback();
            throwDatabaseError(QStringLiteral("cleanup cart item"), cartCleanup.lastError());
        }
    }

    if (!db_.commit()) {
        throwDatabaseError(QStringLiteral("commit checkout"), db_.lastError());
    }

    return true;
}

QVector<WalletRepository::LedgerEntry> SqliteWalletRepository::transactionHistory(const QString& username,
                                                                                   int limit)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();
    ensureWalletRow(username.trimmed());

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT id, username, type, amount_tokens, balance_after, ad_id, counterparty, created_at "
        "FROM transaction_ledger WHERE username = :username "
        "ORDER BY created_at DESC, id DESC LIMIT :limit;"));
    query.bindValue(QStringLiteral(":username"), username.trimmed());
    query.bindValue(QStringLiteral(":limit"), limit > 0 ? limit : 100);

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("transaction history"), query.lastError());
    }

    QVector<LedgerEntry> entries;
    while (query.next()) {
        LedgerEntry entry;
        entry.id = query.value(0).toInt();
        entry.username = query.value(1).toString();
        entry.type = query.value(2).toString();
        entry.amountTokens = query.value(3).toInt();
        entry.balanceAfter = query.value(4).toInt();
        entry.adId = query.value(5).isNull() ? -1 : query.value(5).toInt();
        entry.counterparty = query.value(6).toString();
        entry.createdAt = QDateTime::fromString(query.value(7).toString(), QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        entries.push_back(entry);
    }
    return entries;
}

[[noreturn]] void SqliteWalletRepository::throwDatabaseError(
    const QString& context,
    const QSqlError& error) const
{
    const QString message = QStringLiteral("Database error (%1): %2")
                                .arg(context, error.text());
    throw std::runtime_error(message.toStdString());
}
