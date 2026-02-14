#ifndef KALANET_AD_SERVICE_H
#define KALANET_AD_SERVICE_H

#include <QJsonObject>

#include "protocol/message.h"

class AdRepository;

class AdService
{
public:
    explicit AdService(AdRepository& adRepository);

    common::Message create(const QJsonObject& payload);
    common::Message list(const QJsonObject& payload);
    common::Message detail(const QJsonObject& payload);

private:
    AdRepository& adRepository_;
};

#endif // KALANET_AD_SERVICE_H
