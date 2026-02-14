#ifndef WALLET_SERVICE_H
#define WALLET_SERVICE_H

#include "protocol/message.h"

#include <QSet>
#include <QString>

class WalletRepository;
class CaptchaService;

class WalletService
{
public:
    explicit WalletService(WalletRepository& walletRepository,
                           CaptchaService& captchaService);

    common::Message walletBalance(const QJsonObject& payload);
    common::Message walletTopUp(const QJsonObject& payload);
    common::Message buy(const QJsonObject& payload,
                        QSet<QString>* affectedUsernames,
                        QVector<int>* soldAdIds);
    common::Message transactionHistory(const QJsonObject& payload);

private:
    WalletRepository& walletRepository_;
    CaptchaService& captchaService_;
};

#endif // WALLET_SERVICE_H
