#include "protocol/command_utils.h"

#include <QHash>

namespace common {

namespace {

const QHash<Command, QString>& forwardMap()
{
    static const QHash<Command, QString> map = {
        { Command::Unknown, QStringLiteral("system/unknown") },
        { Command::Ping, QStringLiteral("system/ping") },
        { Command::Pong, QStringLiteral("system/pong") },
        { Command::Error, QStringLiteral("system/error") },

        { Command::Login, QStringLiteral("auth/login/request") },
        { Command::LoginResult, QStringLiteral("auth/login/response") },
        { Command::Signup, QStringLiteral("auth/signup/request") },
        { Command::SignupResult, QStringLiteral("auth/signup/response") },
        { Command::Logout, QStringLiteral("auth/logout/request") },
        { Command::LogoutResult, QStringLiteral("auth/logout/response") },
        { Command::SessionRefresh, QStringLiteral("auth/session/refresh/request") },
        { Command::SessionRefreshResult, QStringLiteral("auth/session/refresh/response") },
        { Command::ProfileUpdate, QStringLiteral("auth/profile/update/request") },
        { Command::ProfileUpdateResult, QStringLiteral("auth/profile/update/response") },
        { Command::ProfileHistory, QStringLiteral("auth/profile/history/request") },
        { Command::ProfileHistoryResult, QStringLiteral("auth/profile/history/response") },
        { Command::AdminStats, QStringLiteral("admin/stats/request") },
        { Command::AdminStatsResult, QStringLiteral("admin/stats/response") },
        { Command::CaptchaChallenge, QStringLiteral("auth/captcha/challenge/request") },
        { Command::CaptchaChallengeResult, QStringLiteral("auth/captcha/challenge/response") },

        { Command::AdCreate, QStringLiteral("ad/create/request") },
        { Command::AdCreateResult, QStringLiteral("ad/create/response") },
        { Command::AdUpdate, QStringLiteral("ad/update/request") },
        { Command::AdUpdateResult, QStringLiteral("ad/update/response") },
        { Command::AdDelete, QStringLiteral("ad/delete/request") },
        { Command::AdDeleteResult, QStringLiteral("ad/delete/response") },
        { Command::AdList, QStringLiteral("ad/list/request") },
        { Command::AdListResult, QStringLiteral("ad/list/response") },
        { Command::AdDetail, QStringLiteral("ad/detail/request") },
        { Command::AdDetailResult, QStringLiteral("ad/detail/response") },
        { Command::AdStatusUpdate, QStringLiteral("ad/status/update") },
        { Command::AdStatusNotify, QStringLiteral("ad/status/notify") },

        { Command::CategoryList, QStringLiteral("category/list/request") },
        { Command::CategoryListResult, QStringLiteral("category/list/response") },

        { Command::CartAddItem, QStringLiteral("cart/add-item/request") },
        { Command::CartAddItemResult, QStringLiteral("cart/add-item/response") },
        { Command::CartRemoveItem, QStringLiteral("cart/remove-item/request") },
        { Command::CartRemoveItemResult, QStringLiteral("cart/remove-item/response") },
        { Command::CartList, QStringLiteral("cart/list/request") },
        { Command::CartListResult, QStringLiteral("cart/list/response") },
        { Command::CartClear, QStringLiteral("cart/clear/request") },
        { Command::CartClearResult, QStringLiteral("cart/clear/response") },

        { Command::Buy, QStringLiteral("purchase/checkout/request") },
        { Command::BuyResult, QStringLiteral("purchase/checkout/response") },
        { Command::TransactionHistory, QStringLiteral("purchase/history/request") },
        { Command::TransactionHistoryResult, QStringLiteral("purchase/history/response") },

        { Command::WalletBalance, QStringLiteral("wallet/balance/request") },
        { Command::WalletBalanceResult, QStringLiteral("wallet/balance/response") },
        { Command::WalletTopUp, QStringLiteral("wallet/topup/request") },
        { Command::WalletTopUpResult, QStringLiteral("wallet/topup/response") },
        { Command::WalletAdjustNotify, QStringLiteral("wallet/adjust/notify") },

        { Command::DiscountCodeValidate, QStringLiteral("discount/validate/request") },
        { Command::DiscountCodeValidateResult, QStringLiteral("discount/validate/response") },
        { Command::DiscountCodeList, QStringLiteral("discount/list/request") },
        { Command::DiscountCodeListResult, QStringLiteral("discount/list/response") },
        { Command::DiscountCodeUpsert, QStringLiteral("discount/upsert/request") },
        { Command::DiscountCodeUpsertResult, QStringLiteral("discount/upsert/response") },
        { Command::DiscountCodeDelete, QStringLiteral("discount/delete/request") },
        { Command::DiscountCodeDeleteResult, QStringLiteral("discount/delete/response") },

        { Command::SystemNotification, QStringLiteral("system/notify") }
    };

    return map;
}

const QHash<QString, Command>& backwardMap()
{
    static const QHash<QString, Command> map = [] {
        QHash<QString, Command> result;
        const auto& forward = forwardMap();
        for (auto it = forward.cbegin(); it != forward.cend(); ++it) {
            result.insert(it.value(), it.key());
        }

        // Backward compatibility aliases (legacy command strings)
        result.insert(QStringLiteral("login"), Command::Login);
        result.insert(QStringLiteral("login_result"), Command::LoginResult);
        result.insert(QStringLiteral("signup"), Command::Signup);
        result.insert(QStringLiteral("signup_result"), Command::SignupResult);
        result.insert(QStringLiteral("logout"), Command::Logout);
        result.insert(QStringLiteral("logout_result"), Command::LogoutResult);
        result.insert(QStringLiteral("buy"), Command::Buy);
        result.insert(QStringLiteral("buy_result"), Command::BuyResult);

        return result;
    }();

    return map;
}

} // namespace

QString commandToString(Command command)
{
    return forwardMap().value(command, QStringLiteral("system/unknown"));
}

Command commandFromString(const QString& commandString)
{
    return backwardMap().value(commandString, Command::Unknown);
}

} // namespace common
