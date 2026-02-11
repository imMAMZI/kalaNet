#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <QJsonObject>

#include "protocol/commands.h"

namespace common::protocol {

    class Message {
    public:
        Message() = default;
        Message(Command cmd, const QJsonObject& payload);

        Command command() const;
        const QJsonObject& payload() const;

        // Serialization
        QJsonObject toJson() const;
        static Message fromJson(const QJsonObject& json);

    private:
        Command command_;
        QJsonObject payload_;
    };

}

#endif // MESSAGE_H
