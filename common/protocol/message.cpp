#include "protocol/message.h"

namespace common::protocol {

    Message::Message(Command cmd, const QJsonObject& payload)
        : command_(cmd), payload_(payload)
    {
    }

    Command Message::command() const {
        return command_;
    }

    const QJsonObject& Message::payload() const {
        return payload_;
    }

    QJsonObject Message::toJson() const {
        QJsonObject json;
        json["command"] = static_cast<int>(command_);
        json["payload"] = payload_;
        return json;
    }

    Message Message::fromJson(const QJsonObject& json) {
        Message msg;
        msg.command_ = static_cast<Command>(json["command"].toInt());
        msg.payload_ = json["payload"].toObject();
        return msg;
    }

}
