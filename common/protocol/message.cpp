#include "protocol/message.h"
#include "protocol/command_utils.h"

namespace common {

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
        json["command"] = commandToString(command_);
        json["payload"] = payload_;
        return json;
    }

    Message Message::fromJson(const QJsonObject& json) {
        Message msg;
        msg.command_ = stringToCommand(json["command"].toString());
        msg.payload_ = json["payload"].toObject();
        return msg;
    }

}
