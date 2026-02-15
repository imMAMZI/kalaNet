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

    QSqlQuery createDiscountCodes(db_);
    if (!createDiscountCodes.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS discount_codes ("
            " code TEXT PRIMARY KEY,"
            " type TEXT NOT NULL CHECK(type IN ('percent','fixed')) ,"
            " value_tokens INTEGER NOT NULL CHECK(value_tokens > 0),"
            " max_discount_tokens INTEGER NOT NULL DEFAULT 0,"
            " min_subtotal_tokens INTEGER NOT NULL DEFAULT 0,"
            " usage_limit INTEGER,"
            " used_count INTEGER NOT NULL DEFAULT 0,"
            " is_active INTEGER NOT NULL DEFAULT 1,"
            " expires_at TEXT,"
            " created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            " updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
            ");"))) {
        throwDatabaseError(QStringLiteral("create discount_codes table"), createDiscountCodes.lastError());
    }

    QSqlQuery createDiscountIndex(db_);
    if (!createDiscountIndex.exec(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_discount_codes_active ON discount_codes(is_active, code);"))) {
        throwDatabaseError(QStringLiteral("create discount_codes index"), createDiscountIndex.lastError());
    }

    QSqlQuery seedDiscount(db_);
    if (!seedDiscount.exec(QStringLiteral(
            "INSERT OR IGNORE INTO discount_codes(code, type, value_tokens, max_discount_tokens, min_subtotal_tokens, usage_limit, is_active) "
            "VALUES "
            "('OFF10', 'percent', 10, 50, 0, NULL, 1),"
            "('OFF20', 'percent', 20, 100, 0, NULL, 1);"))) {
        throwDatabaseError(QStringLiteral("seed discount_codes"), seedDiscount.lastError());
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

WalletRepository::DiscountValidationResult SqliteWalletRepository::validateDiscountCode(const QString& code,
                                                                                         int subtotalTokens,
                                                                                         const QString&)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    DiscountValidationResult result;
    result.subtotalTokens = qMax(0, subtotalTokens);
    result.totalTokens = result.subtotalTokens;
    result.code = code.trimmed().toUpper();

    if (result.code.isEmpty()) {
        result.message = QStringLiteral("No discount code provided");
        return result;
    }

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "SELECT code, type, value_tokens, max_discount_tokens, min_subtotal_tokens, usage_limit, used_count, is_active, expires_at "
        "FROM discount_codes WHERE code = :code LIMIT 1;"));
    query.bindValue(QStringLiteral(":code"), result.code);

    if (!query.exec()) {
        throwDatabaseError(QStringLiteral("validate discount code"), query.lastError());
    }

    if (!query.next()) {
        result.message = QStringLiteral("Discount code not found");
        return result;
    }

    result.type = query.value(1).toString().trimmed().toLower();
    result.valueTokens = query.value(2).toInt();
    result.maxDiscountTokens = qMax(0, query.value(3).toInt());
    result.minSubtotalTokens = qMax(0, query.value(4).toInt());
    result.usageLimit = query.value(5).isNull() ? -1 : query.value(5).toInt();
    result.usedCount = query.value(6).toInt();
    result.active = query.value(7).toInt() != 0;
    result.expiresAt = QDateTime::fromString(query.value(8).toString(), Qt::ISODate);

    if (!result.active) {
        result.message = QStringLiteral("Discount code is inactive");
        return result;
    }

    if (result.expiresAt.isValid() && result.expiresAt < QDateTime::currentDateTimeUtc()) {
        result.message = QStringLiteral("Discount code has expired");
        return result;
    }

    if (result.usageLimit >= 0 && result.usedCount >= result.usageLimit) {
        result.message = QStringLiteral("Discount code usage limit reached");
        return result;
    }

    if (result.subtotalTokens < result.minSubtotalTokens) {
        result.message = QStringLiteral("Subtotal must be at least %1 tokens").arg(result.minSubtotalTokens);
        return result;
    }

    int discount = 0;
    if (result.type == QStringLiteral("percent")) {
        discount = (result.subtotalTokens * result.valueTokens) / 100;
    } else if (result.type == QStringLiteral("fixed")) {
        discount = result.valueTokens;
    } else {
        result.message = QStringLiteral("Discount code has invalid type");
        return result;
    }

    if (result.maxDiscountTokens > 0) {
        discount = qMin(discount, result.maxDiscountTokens);
    }
    discount = qMax(0, qMin(discount, result.subtotalTokens));

    result.discountTokens = discount;
    result.totalTokens = result.subtotalTokens - discount;
    result.valid = discount > 0;
    result.message = result.valid
                         ? QStringLiteral("Discount code applied")
                         : QStringLiteral("Discount code does not reduce this cart");
    return result;
}

QVector<WalletRepository::DiscountValidationResult> SqliteWalletRepository::listDiscountCodes()
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    if (!query.exec(QStringLiteral(
            "SELECT code, type, value_tokens, max_discount_tokens, min_subtotal_tokens, usage_limit, used_count, is_active, expires_at "
            "FROM discount_codes ORDER BY code ASC;"))) {
        throwDatabaseError(QStringLiteral("list discount codes"), query.lastError());
    }

    QVector<DiscountValidationResult> rows;
    while (query.next()) {
        DiscountValidationResult item;
        item.code = query.value(0).toString();
        item.type = query.value(1).toString();
        item.valueTokens = query.value(2).toInt();
        item.maxDiscountTokens = query.value(3).toInt();
        item.minSubtotalTokens = query.value(4).toInt();
        item.usageLimit = query.value(5).isNull() ? -1 : query.value(5).toInt();
        item.usedCount = query.value(6).toInt();
        item.active = query.value(7).toInt() != 0;
        item.expiresAt = QDateTime::fromString(query.value(8).toString(), Qt::ISODate);
        rows.push_back(item);
    }
    return rows;
}

bool SqliteWalletRepository::upsertDiscountCode(const DiscountValidationResult& record,
                                                QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    const QString code = record.code.trimmed().toUpper();
    const QString type = record.type.trimmed().toLower();
    if (code.isEmpty() || (type != QStringLiteral("percent") && type != QStringLiteral("fixed")) || record.valueTokens <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Invalid discount code payload");
        }
        return false;
    }

    if (type == QStringLiteral("percent") && record.valueTokens > 100) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Percent discount cannot exceed 100");
        }
        return false;
    }

    QSqlQuery query(db_);
    query.prepare(QStringLiteral(
        "INSERT INTO discount_codes(code, type, value_tokens, max_discount_tokens, min_subtotal_tokens, usage_limit, used_count, is_active, expires_at, updated_at) "
        "VALUES(:code, :type, :value, :max_discount, :min_subtotal, :usage_limit, :used_count, :is_active, :expires_at, CURRENT_TIMESTAMP) "
        "ON CONFLICT(code) DO UPDATE SET "
        " type = excluded.type,"
        " value_tokens = excluded.value_tokens,"
        " max_discount_tokens = excluded.max_discount_tokens,"
        " min_subtotal_tokens = excluded.min_subtotal_tokens,"
        " usage_limit = excluded.usage_limit,"
        " is_active = excluded.is_active,"
        " expires_at = excluded.expires_at,"
        " updated_at = CURRENT_TIMESTAMP;"));

    query.bindValue(QStringLiteral(":code"), code);
    query.bindValue(QStringLiteral(":type"), type);
    query.bindValue(QStringLiteral(":value"), record.valueTokens);
    query.bindValue(QStringLiteral(":max_discount"), qMax(0, record.maxDiscountTokens));
    query.bindValue(QStringLiteral(":min_subtotal"), qMax(0, record.minSubtotalTokens));
    if (record.usageLimit < 0) {
        query.bindValue(QStringLiteral(":usage_limit"), QVariant(QVariant::Int));
    } else {
        query.bindValue(QStringLiteral(":usage_limit"), record.usageLimit);
    }
    query.bindValue(QStringLiteral(":used_count"), qMax(0, record.usedCount));
    query.bindValue(QStringLiteral(":is_active"), record.active ? 1 : 0);
    if (record.expiresAt.isValid()) {
        query.bindValue(QStringLiteral(":expires_at"), record.expiresAt.toUTC().toString(Qt::ISODate));
    } else {
        query.bindValue(QStringLiteral(":expires_at"), QVariant(QVariant::String));
    }

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }
    return true;
}

bool SqliteWalletRepository::deleteDiscountCode(const QString& code,
                                                QString* errorMessage)
{
    QMutexLocker locker(&mutex_);
    ensureConnection();

    QSqlQuery query(db_);
    query.prepare(QStringLiteral("DELETE FROM discount_codes WHERE code = :code;"));
    query.bindValue(QStringLiteral(":code"), code.trimmed().toUpper());

    if (!query.exec()) {
        if (errorMessage) {
            *errorMessage = query.lastError().text();
        }
        return false;
    }

    if (query.numRowsAffected() != 1) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Discount code not found");
        }
        return false;
    }

    return true;
}

bool SqliteWalletRepository::checkout(const QString& buyerUsername,
                                      const QVector<int>& adIds,
                                      const QString& discountCode,
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
    int subtotal = 0;

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

        subtotal += price;
        sellerCredits[seller] += price;

        CheckoutItem item;
        item.adId = adId;
        item.priceTokens = price;
        item.sellerUsername = seller;
        result.purchasedItems.push_back(item);
    }

    const QString normalizedCode = discountCode.trimmed().toUpper();
    DiscountValidationResult discountResult;
    if (!normalizedCode.isEmpty()) {
        discountResult.code = normalizedCode;
        discountResult.subtotalTokens = subtotal;

        QSqlQuery discountQuery(db_);
        discountQuery.prepare(QStringLiteral(
            "SELECT type, value_tokens, max_discount_tokens, min_subtotal_tokens, usage_limit, used_count, is_active, expires_at "
            "FROM discount_codes WHERE code = :code LIMIT 1;"));
        discountQuery.bindValue(QStringLiteral(":code"), normalizedCode);
        if (!discountQuery.exec()) {
            db_.rollback();
            throwDatabaseError(QStringLiteral("load discount for checkout"), discountQuery.lastError());
        }

        if (!discountQuery.next()) {
            db_.rollback();
            if (errorMessage) {
                *errorMessage = QStringLiteral("Discount code not found");
            }
            return false;
        }

        discountResult.type = discountQuery.value(0).toString().trimmed().toLower();
        discountResult.valueTokens = discountQuery.value(1).toInt();
        discountResult.maxDiscountTokens = qMax(0, discountQuery.value(2).toInt());
        discountResult.minSubtotalTokens = qMax(0, discountQuery.value(3).toInt());
        discountResult.usageLimit = discountQuery.value(4).isNull() ? -1 : discountQuery.value(4).toInt();
        discountResult.usedCount = discountQuery.value(5).toInt();
        discountResult.active = discountQuery.value(6).toInt() != 0;
        discountResult.expiresAt = QDateTime::fromString(discountQuery.value(7).toString(), Qt::ISODate);

        if (!discountResult.active) {
            db_.rollback();
            if (errorMessage) {
                *errorMessage = QStringLiteral("Discount code is inactive");
            }
            return false;
        }

        if (discountResult.expiresAt.isValid() && discountResult.expiresAt < QDateTime::currentDateTimeUtc()) {
            db_.rollback();
            if (errorMessage) {
                *errorMessage = QStringLiteral("Discount code has expired");
            }
            return false;
        }

        if (discountResult.usageLimit >= 0 && discountResult.usedCount >= discountResult.usageLimit) {
            db_.rollback();
            if (errorMessage) {
                *errorMessage = QStringLiteral("Discount code usage limit reached");
            }
            return false;
        }

        if (subtotal < discountResult.minSubtotalTokens) {
            db_.rollback();
            if (errorMessage) {
                *errorMessage = QStringLiteral("Subtotal must be at least %1 tokens").arg(discountResult.minSubtotalTokens);
            }
            return false;
        }

        if (discountResult.type == QStringLiteral("percent")) {
            discountResult.discountTokens = (subtotal * discountResult.valueTokens) / 100;
        } else if (discountResult.type == QStringLiteral("fixed")) {
            discountResult.discountTokens = discountResult.valueTokens;
        }
        if (discountResult.maxDiscountTokens > 0) {
            discountResult.discountTokens = qMin(discountResult.discountTokens, discountResult.maxDiscountTokens);
        }
        discountResult.discountTokens = qMax(0, qMin(discountResult.discountTokens, subtotal));
        discountResult.valid = discountResult.discountTokens > 0;

        if (!discountResult.valid) {
            db_.rollback();
            if (errorMessage) {
                *errorMessage = QStringLiteral("Discount code does not reduce this cart");
            }
            return false;
        }

        QSqlQuery consumeCode(db_);
        consumeCode.prepare(QStringLiteral(
            "UPDATE discount_codes "
            "SET used_count = used_count + 1, updated_at = CURRENT_TIMESTAMP "
            "WHERE code = :code AND is_active = 1 AND (usage_limit IS NULL OR used_count < usage_limit);"));
        consumeCode.bindValue(QStringLiteral(":code"), normalizedCode);
        if (!consumeCode.exec() || consumeCode.numRowsAffected() != 1) {
            db_.rollback();
            if (errorMessage) {
                *errorMessage = QStringLiteral("Discount code could not be consumed");
            }
            return false;
        }
    }

    const int totalCost = qMax(0, subtotal - discountResult.discountTokens);

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

    result.subtotalTokens = subtotal;
    result.discountTokens = discountResult.discountTokens;
    result.totalTokens = totalCost;
    result.appliedDiscountCode = normalizedCode;
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

        QSqlQuery adHistory(db_);
        adHistory.prepare(QStringLiteral(
            "INSERT INTO ad_status_history (ad_id, previous_status, new_status, reason) "
            "VALUES (:ad_id, 'approved', 'sold', :reason);"));
        adHistory.bindValue(QStringLiteral(":ad_id"), item.adId);
        adHistory.bindValue(QStringLiteral(":reason"),
                            QStringLiteral("sold to %1").arg(buyer));
        if (!adHistory.exec()) {
            db_.rollback();
            throwDatabaseError(QStringLiteral("insert sold ad history"), adHistory.lastError());
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

    if (result.discountTokens > 0) {
        QSqlQuery discountLedger(db_);
        discountLedger.prepare(QStringLiteral(
            "INSERT INTO transaction_ledger(username, type, amount_tokens, balance_after, ad_id, counterparty) "
            "VALUES(:username, 'discount_credit', :amount, "
            "(SELECT balance_tokens FROM wallets WHERE username = :username), NULL, :counterparty);"));
        discountLedger.bindValue(QStringLiteral(":username"), buyer);
        discountLedger.bindValue(QStringLiteral(":amount"), result.discountTokens);
        discountLedger.bindValue(QStringLiteral(":counterparty"), result.appliedDiscountCode);
        if (!discountLedger.exec()) {
            db_.rollback();
            throwDatabaseError(QStringLiteral("insert discount ledger"), discountLedger.lastError());
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
