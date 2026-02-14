#ifndef KALANET_CART_SERVICE_H
#define KALANET_CART_SERVICE_H

#include <QJsonObject>

#include "protocol/message.h"

class CartRepository;
class AdRepository;

class CartService
{
public:
    CartService(CartRepository& cartRepository,
                AdRepository& adRepository);

    common::Message addItem(const QJsonObject& payload);
    common::Message removeItem(const QJsonObject& payload);
    common::Message list(const QJsonObject& payload);
    common::Message clear(const QJsonObject& payload);

private:
    CartRepository& cartRepository_;
    AdRepository& adRepository_;
};

#endif // KALANET_CART_SERVICE_H
