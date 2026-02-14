#ifndef AD_REPOSITORY_H
#define AD_REPOSITORY_H

#include <QByteArray>
#include <QString>

class AdRepository
{
public:
    struct NewAd {
        QString title;
        QString description;
        QString category;
        int priceTokens = 0;
        QString sellerUsername;
        QByteArray imageBytes;
    };

    virtual ~AdRepository() = default;

    virtual int createPendingAd(const NewAd& ad) = 0;
};

#endif // AD_REPOSITORY_H
