#include "protocol/serializer.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

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

// --------------------------------------------
//  Command → String
// --------------------------------------------
QString Serializer::commandToString(Command cmd) {
    switch (cmd) {
        case Command::Login: return "Login";
        case Command::Signup: return "Signup";
        case Command::GetAds: return "GetAds";
        case Command::AddToCart: return "AddToCart";
        case Command::RemoveFromCart: return "RemoveFromCart";
        case Command::Buy: return "Buy";
        case Command::BuyResult: return "BuyResult";
        // add other commands here...
    }
    return "Unknown";
}

// --------------------------------------------
//  String → Command
// --------------------------------------------
Command Serializer::stringToCommand(const QString& str) {
    if (str == "Login") return Command::Login;
    if (str == "Signup") return Command::Signup;
    if (str == "GetAds") return Command::GetAds;
    if (str == "AddToCart") return Command::AddToCart;
    if (str == "RemoveFromCart") return Command::RemoveFromCart;
    if (str == "Buy") return Command::Buy;
    if (str == "BuyResult") return Command::BuyResult;

    throw std::runtime_error("Unknown command: " + str.toStdString());
}

}
