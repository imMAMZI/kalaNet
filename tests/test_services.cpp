#include <QtTest/QtTest>

#include "server/auth/auth_service.h"
#include "server/ads/ad_service.h"
#include "server/cart/cart_service.h"
#include "server/wallet/wallet_service.h"
#include "server/repository/sqlite_wallet_repository.h"
#include "server/repository/sqlite_ad_repository.h"
#include "server/repository/sqlite_cart_repository.h"

#include <QDir>
#include <QFile>
#include <QUuid>

#include <thread>

class InMemoryUserRepository final : public UserRepository {
public:
    QHash<QString, User> users;

    bool userExists(const QString& username) override { return users.contains(username); }
    bool emailExists(const QString& email) override {
        for (const auto& u : users) if (u.email == email) return true;
        return false;
    }
    bool checkPassword(const QString&, const QString&) override { return false; }
    bool getUser(const QString& username, User& outUser) override {
        if (!users.contains(username)) return false;
        outUser = users.value(username);
        return true;
    }
    void createUser(const User& user) override { users.insert(user.username, user); }
    bool updateUser(const QString&, const User&) override { return false; }
    int countAllUsers() override { return users.size(); }
    int countUsersByRole(const QString& role) override {
        int c = 0; for (const auto& u : users) if (u.role == role) ++c; return c;
    }
};

class InMemoryAdRepository final : public AdRepository {
public:
    QHash<int, AdDetailRecord> ads;
    bool duplicate = false;
    int nextId = 1;

    int createPendingAd(const NewAd& ad) override {
        AdDetailRecord record;
        record.id = nextId++;
        record.title = ad.title;
        record.description = ad.description;
        record.category = ad.category;
        record.priceTokens = ad.priceTokens;
        record.sellerUsername = ad.sellerUsername;
        record.status = QStringLiteral("pending");
        ads.insert(record.id, record);
        return record.id;
    }
    QVector<AdSummaryRecord> listApprovedAds(const AdListFilters&) override { return {}; }
    std::optional<AdDetailRecord> findApprovedAdById(int adId) override {
        if (!ads.contains(adId) || ads.value(adId).status != QStringLiteral("approved")) return std::nullopt;
        return ads.value(adId);
    }
    std::optional<AdDetailRecord> findAdById(int adId) override {
        if (!ads.contains(adId)) return std::nullopt;
        return ads.value(adId);
    }
    bool hasDuplicateActiveAdForSeller(const NewAd&) override { return duplicate; }
    bool updateStatus(int adId, AdModerationStatus newStatus, const QString&) override {
        if (!ads.contains(adId)) return false;
        ads[adId].status = (newStatus == AdModerationStatus::Approved) ? QStringLiteral("approved") : QStringLiteral("rejected");
        return true;
    }
    QVector<AdSummaryRecord> listAdsBySeller(const QString&, const QString&) override { return {}; }
    QVector<AdSummaryRecord> listPurchasedAdsByBuyer(const QString&, int) override { return {}; }
    AdStatusCounts getAdStatusCounts() override { return {}; }
    SalesTotals getSalesTotals() override { return {}; }
};

class InMemoryCartRepository final : public CartRepository {
public:
    QSet<int> items;
    bool addItem(const QString&, int adId) override { bool existed = items.contains(adId); items.insert(adId); return !existed; }
    bool removeItem(const QString&, int adId) override { return items.remove(adId) > 0; }
    QVector<int> listItems(const QString&) override { return items.values().toVector(); }
    int clearItems(const QString&) override { int s = items.size(); items.clear(); return s; }
    bool hasItem(const QString&, int adId) override { return items.contains(adId); }
};

class InMemoryWalletRepository final : public WalletRepository {
public:
    int balance = 0;
    bool checkoutSuccess = false;
    QString checkoutErr;

    int getBalance(const QString&) override { return balance; }
    int topUp(const QString&, int amountTokens) override { balance += amountTokens; return balance; }
    bool checkout(const QString&, const QVector<int>&, CheckoutResult&, QString* errorMessage) override {
        if (!checkoutSuccess && errorMessage) *errorMessage = checkoutErr;
        return checkoutSuccess;
    }
    QVector<LedgerEntry> transactionHistory(const QString&, int) override { return {}; }
};

class ServiceRulesTests : public QObject {
    Q_OBJECT

private slots:
    void authLoginInvalidCredentials();
    void adsRejectDuplicate();
    void cartRejectSoldItems();
    void walletMapsInsufficientFunds();
    void checkoutRulesAndAtomicUpdates();
    void walletTopUpIsThreadSafe();
};

void ServiceRulesTests::authLoginInvalidCredentials()
{
    InMemoryUserRepository users;
    User user{QStringLiteral("A"), QStringLiteral("alice"), QStringLiteral("1"), QStringLiteral("a@a.com"), PasswordHasher::hash(QStringLiteral("Password123")), QStringLiteral("user")};
    users.createUser(user);
    AuthService service(users);

    const common::Message response = service.login(QJsonObject{{QStringLiteral("username"), QStringLiteral("alice")},
                                                               {QStringLiteral("password"), QStringLiteral("wrong")}});
    QVERIFY(response.isFailure());
    QCOMPARE(response.errorCode(), common::ErrorCode::AuthInvalidCredentials);
    QCOMPARE(response.payload().value(QStringLiteral("statusCode")).toInt(), 401);
}

void ServiceRulesTests::adsRejectDuplicate()
{
    InMemoryAdRepository ads;
    ads.duplicate = true;
    AdService service(ads);

    const common::Message response = service.create(QJsonObject{{QStringLiteral("title"), QStringLiteral("Gaming PC")},
                                                                {QStringLiteral("description"), QStringLiteral("Strong machine for gaming")},
                                                                {QStringLiteral("category"), QStringLiteral("Electronics")},
                                                                {QStringLiteral("priceTokens"), 100},
                                                                {QStringLiteral("sellerUsername"), QStringLiteral("seller")}});
    QVERIFY(response.isFailure());
    QCOMPARE(response.errorCode(), common::ErrorCode::DuplicateAd);
    QCOMPARE(response.payload().value(QStringLiteral("statusCode")).toInt(), 409);
}

void ServiceRulesTests::cartRejectSoldItems()
{
    InMemoryAdRepository ads;
    AdRepository::NewAd newAd{QStringLiteral("Item"), QStringLiteral("description long enough"), QStringLiteral("Cat"), 10, QStringLiteral("seller"), {}};
    const int adId = ads.createPendingAd(newAd);
    ads.ads[adId].status = QStringLiteral("sold");

    InMemoryCartRepository carts;
    CartService service(carts, ads);

    const common::Message response = service.addItem(QJsonObject{{QStringLiteral("username"), QStringLiteral("buyer")},
                                                                 {QStringLiteral("adId"), adId}});
    QVERIFY(response.isFailure());
    QCOMPARE(response.errorCode(), common::ErrorCode::AdNotAvailable);
}

void ServiceRulesTests::walletMapsInsufficientFunds()
{
    InMemoryWalletRepository wallet;
    wallet.checkoutErr = QStringLiteral("Insufficient wallet balance");
    WalletService service(wallet);

    const common::Message response = service.buy(QJsonObject{{QStringLiteral("username"), QStringLiteral("buyer")},
                                                             {QStringLiteral("adIds"), QJsonArray{1}}});
    QVERIFY(response.isFailure());
    QCOMPARE(response.errorCode(), common::ErrorCode::InsufficientFunds);
    QCOMPARE(response.payload().value(QStringLiteral("statusCode")).toInt(), 402);
}

static QString tempDbPath()
{
    return QDir::tempPath() + QDir::separator() + QStringLiteral("kalanet_test_") + QUuid::createUuid().toString(QUuid::WithoutBraces) + QStringLiteral(".db");
}

void ServiceRulesTests::checkoutRulesAndAtomicUpdates()
{
    const QString dbPath = tempDbPath();
    {
        SqliteAdRepository adRepo(dbPath);
        SqliteCartRepository cartRepo(dbPath);
        SqliteWalletRepository walletRepo(dbPath);

        AdRepository::NewAd ad{QStringLiteral("Phone"), QStringLiteral("A nice phone for sale"), QStringLiteral("Electronics"), 50, QStringLiteral("seller"), {}};
        const int adId = adRepo.createPendingAd(ad);
        QVERIFY(adRepo.updateStatus(adId, AdRepository::AdModerationStatus::Approved, QStringLiteral("ok")));

        walletRepo.topUp(QStringLiteral("buyer"), 100);
        WalletRepository::CheckoutResult result;
        QString err;
        QVERIFY(walletRepo.checkout(QStringLiteral("buyer"), QVector<int>{adId}, result, &err));

        QCOMPARE(walletRepo.getBalance(QStringLiteral("buyer")), 50);
        QCOMPARE(walletRepo.getBalance(QStringLiteral("seller")), 50);
        const auto adDetails = adRepo.findAdById(adId);
        QVERIFY(adDetails.has_value());
        QCOMPARE(adDetails->status.toLower(), QStringLiteral("sold"));

        // buying sold item must fail
        QVERIFY(!walletRepo.checkout(QStringLiteral("buyer"), QVector<int>{adId}, result, &err));
    }
    QFile::remove(dbPath);
}

void ServiceRulesTests::walletTopUpIsThreadSafe()
{
    const QString dbPath = tempDbPath();
    {
        SqliteWalletRepository walletRepo(dbPath);
        constexpr int threads = 8;
        constexpr int iterations = 40;
        std::vector<std::thread> workers;
        workers.reserve(threads);

        for (int t = 0; t < threads; ++t) {
            workers.emplace_back([&walletRepo]() {
                for (int i = 0; i < iterations; ++i) {
                    walletRepo.topUp(QStringLiteral("concurrent"), 1);
                }
            });
        }

        for (auto& w : workers) {
            w.join();
        }

        QCOMPARE(walletRepo.getBalance(QStringLiteral("concurrent")), threads * iterations);
    }
    QFile::remove(dbPath);
}

QTEST_MAIN(ServiceRulesTests)
#include "test_services.moc"
