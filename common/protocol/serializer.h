#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <QByteArray>
#include <QString>
#include <QJsonObject>
#include "protocol/message.h"
#include "protocol/commands.h"

namespace common {

    class Serializer {
    public:
        // Convert Message → QByteArray(JSON)
        static QByteArray serialize(const Message& message);

        // Convert QByteArray(JSON) → Message
        static Message deserialize(const QByteArray& data);

    private:
        // Helpers for command mapping
        static QString commandToString(Command cmd);
        static Command stringToCommand(const QString& str);
    };

}

#endif // SERIALIZER_H
