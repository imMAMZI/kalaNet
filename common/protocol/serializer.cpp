#include "protocol/serializer.h"
#include "protocol/command_utils.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace common {

    QByteArray Serializer::serialize(const Message& message) {
        QJsonObject root;
        root["command"] = commandToString(message.command());
        root["payload"] = message.payload();

        QJsonDocument doc(root);
        return doc.toJson(QJsonDocument::Compact);
    }

    Message Serializer::deserialize(const QByteArray& data) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isObject()) {
            throw std::runtime_error("Invalid JSON message");
        }

        QJsonObject root = doc.object();

        if (!root.contains("command") || !root.contains("payload")) {
            throw std::runtime_error("Missing fields in message");
        }

        QString cmdStr = root["command"].toString();
        Command cmd = stringToCommand(cmdStr);

        QJsonObject payload = root["payload"].toObject();

        return Message(cmd, payload);
    }

}
