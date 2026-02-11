#include "protocol/buy_message.h"
#include <QJsonArray>

namespace common {

    Message BuyMessage::createRequest(
            const std::string& username,
            const std::vector<int>& cart)
    {
        QJsonObject payload;
        payload["username"] = QString::fromStdString(username);

        QJsonArray arr;
        for (int id : cart)
            arr.append(id);

        payload["cart"] = arr;

        return Message(Command::Buy, payload);
    }

    Message BuyMessage::createSuccessResponse(
            int transactionId,
            double updatedWallet,
            const std::vector<int>& soldAds)
    {
        QJsonObject payload;
        payload["success"] = true;
        payload["message"] = "Purchase completed successfully";
        payload["transactionId"] = transactionId;
        payload["updatedWallet"] = updatedWallet;

        QJsonArray arr;
        for (int id : soldAds)
            arr.append(id);

        payload["soldAds"] = arr;

        return Message(Command::BuyResult, payload);
    }

    Message BuyMessage::createFailureResponse(
            const std::string& reason,
            const std::vector<int>& invalidAds)
    {
        QJsonObject payload;
        payload["success"] = false;
        payload["message"] = QString::fromStdString(reason);

        if (!invalidAds.empty()) {
            QJsonArray arr;
            for (int id : invalidAds)
                arr.append(id);
            payload["invalidAds"] = arr;
        }

        return Message(Command::BuyResult, payload);
    }

}
