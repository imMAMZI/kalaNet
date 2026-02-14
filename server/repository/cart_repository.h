#ifndef CART_REPOSITORY_H
#define CART_REPOSITORY_H

#include <QVector>
#include <QString>

class CartRepository
{
public:
    virtual ~CartRepository() = default;

    virtual bool addItem(const QString& username, int adId) = 0;
    virtual bool removeItem(const QString& username, int adId) = 0;
    virtual QVector<int> listItems(const QString& username) = 0;
    virtual int clearItems(const QString& username) = 0;
    virtual bool hasItem(const QString& username, int adId) = 0;
};

#endif // CART_REPOSITORY_H
