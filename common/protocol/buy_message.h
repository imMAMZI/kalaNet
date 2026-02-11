#ifndef BUY_MESSAGE_H
#define BUY_MESSAGE_H

#include "protocol/message.h"
#include "protocol/commands.h"
#include <vector>

namespace common {
    class BuyMessage {
    public:
        static Message createRequest(
            const std::string& username,
            const std::vector<int>& cart
        );

        static Message createSuccessResponse(
            int transactionId,
            double updatedWallet,
            const std::vector<int>& soldAds
        );

        static Message createFailureResponse(
            const std::string& reason,
            const std::vector<int>& invalidAds = {}
        );
    };

}

#endif
