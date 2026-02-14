#ifndef KALANET_AD_SERVICE_H
#define KALANET_AD_SERVICE_H

#include <QJsonObject>
#include <QMutex>
#include <QVector>

#include "protocol/message.h"

class AdService
{
public:
    common::Message create(const QJsonObject& payload);

private:
    struct StoredAd {
        int id = -1;
        QString title;
        QString description;
        QString category;
        int priceTokens = 0;
        QString sellerUsername;
        QByteArray imageBytes;
    };

    int nextAdId_ = 1;
    QVector<StoredAd> pendingAds_;
    QMutex mutex_;
};

#endif // KALANET_AD_SERVICE_H

