#include "message.h"
#include "protocol/command_utils.h"

using namespace common;

Message::Message(Command cmd, const QJsonObject& payload)
    : command_(cmd), payload_(payload)
{
}

Command Message::command() const
{
    return command_;
}

const QJsonObject& Message::payload() const
{
    return payload_;
}

QJsonObject Message::toJson() const
{
    QJsonObject obj;
    obj["command"] = commandToString(command_);
    obj["payload"] = payload_;
    return obj;
}

Message Message::fromJson(const QJsonObject& json)
{
    const QString cmdStr = json.value("command").toString();
    Command cmd = stringToCommand(cmdStr);

    QJsonObject payload = json.value("payload").toObject();
    return Message(cmd, payload);
}
