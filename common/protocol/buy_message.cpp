#include "protocol/buy_message.h"

#include <QJsonArray>

namespace common {

Message BuyMessage::createRequest(const std::string& username,
                                  const std::vector<int>& cart,
                                  const QString& requestId,
                                  const QString& sessionToken)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("username"), QString::fromStdString(username));

    QJsonArray cartArray;
    for (int id : cart) {
        cartArray.append(id);
    }
    payload.insert(QStringLiteral("cart"), cartArray);

    return Message(Command::Buy, payload, requestId, sessionToken);
}

Message BuyMessage::createSuccessResponse(int transactionId,
                                          double updatedWallet,
                                          const std::vector<int>& soldAds,
                                          const QString& requestId,
                                          const QString& sessionToken,
                                          const QString& statusMessage)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("success"), true);
    payload.insert(QStringLiteral("transactionId"), transactionId);
    payload.insert(QStringLiteral("updatedWallet"), updatedWallet);

    QJsonArray soldArray;
    for (int id : soldAds) {
        soldArray.append(id);
    }
    payload.insert(QStringLiteral("soldAds"), soldArray);

    return Message::makeSuccess(Command::BuyResult, payload, requestId, sessionToken, statusMessage);
}

Message BuyMessage::createFailureResponse(ErrorCode errorCode,
                                          const QString& reason,
                                          const std::vector<int>& invalidAds,
                                          const QString& requestId,
                                          const QString& sessionToken)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("success"), false);
    payload.insert(QStringLiteral("reason"), reason);

    if (!invalidAds.empty()) {
        QJsonArray invalidArray;
        for (int id : invalidAds) {
            invalidArray.append(id);
        }
        payload.insert(QStringLiteral("invalidAds"), invalidArray);
    }

    return Message::makeFailure(Command::BuyResult, errorCode, reason, payload, requestId, sessionToken);
}

} // namespace common