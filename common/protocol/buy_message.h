#ifndef BUY_MESSAGE_H
#define BUY_MESSAGE_H

#include <vector>

#include <QString>

#include "protocol/error_codes.h"
#include "protocol/message.h"

namespace common {

    class BuyMessage {
    public:
        static Message createRequest(const std::string& username,
                                     const std::vector<int>& cart,
                                     const QString& requestId = {},
                                     const QString& sessionToken = {});

        static Message createSuccessResponse(int transactionId,
                                             double updatedWallet,
                                             const std::vector<int>& soldAds,
                                             const QString& requestId = {},
                                             const QString& sessionToken = {},
                                             const QString& statusMessage = QStringLiteral("Purchase completed successfully"));

        static Message createFailureResponse(ErrorCode errorCode,
                                             const QString& reason,
                                             const std::vector<int>& invalidAds = {},
                                             const QString& requestId = {},
                                             const QString& sessionToken = {});
    };

}

#endif // BUY_MESSAGE_H
