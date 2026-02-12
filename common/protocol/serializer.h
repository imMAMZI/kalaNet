#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <QByteArray>
#include <QJsonObject>
#include "protocol/message.h"

namespace common {

    class Serializer {
    public:
        static QByteArray serialize(const Message& message);
        static Message deserialize(const QByteArray& data);
    };

}

#endif // SERIALIZER_H
