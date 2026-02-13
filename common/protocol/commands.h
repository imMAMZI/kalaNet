#ifndef COMMON_PROTOCOL_COMMANDS_H
#define COMMON_PROTOCOL_COMMANDS_H

#include <QMetaType>

namespace common {

    enum class Command {
        Unknown = 0,
        Ping,
        Pong,
        Error,

        Login,
        LoginResult,
        Signup,
        SignupResult,
        Logout,
        LogoutResult,
        SessionRefresh,
        SessionRefreshResult,

        AdCreate,
        AdCreateResult,
        AdUpdate,
        AdUpdateResult,
        AdDelete,
        AdDeleteResult,
        AdList,
        AdListResult,
        AdDetail,
        AdDetailResult,
        AdStatusUpdate,
        AdStatusNotify,

        CategoryList,
        CategoryListResult,

        CartAddItem,
        CartAddItemResult,
        CartRemoveItem,
        CartRemoveItemResult,
        CartList,
        CartListResult,
        CartClear,
        CartClearResult,

        Buy,
        BuyResult,
        TransactionHistory,
        TransactionHistoryResult,

        WalletBalance,
        WalletBalanceResult,
        WalletTopUp,
        WalletTopUpResult,
        WalletAdjustNotify,

        SystemNotification
    };

} // namespace common

Q_DECLARE_METATYPE(common::Command)

#endif // COMMON_PROTOCOL_COMMANDS_H
