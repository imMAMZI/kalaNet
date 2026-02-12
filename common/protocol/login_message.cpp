#include "protocol/login_message.h"
#include "protocol/commands.h"
#include <QJsonObject>
#include <QString>

namespace common {

    Message LoginMessage::createRequest(
        const std::string& username,
        const std::string& password)
    {
        QJsonObject payload;
        payload["username"] = QString::fromStdString(username);
        payload["password"] = QString::fromStdString(password);

        return Message(Command::Login, payload);
    }

    Message LoginMessage::createSuccessResponse(
        const std::string& fullName,
        const std::string& role)
    {
        QJsonObject payload;
        payload["success"] = true;
        payload["message"] = "Login Successful";
        payload["fullName"] = QString::fromStdString(fullName);
        payload["role"] = QString::fromStdString(role);

        return Message(Command::LoginResult, payload);
    }

    Message LoginMessage::createFailureResponse(
        const std::string& reason)
    {
        QJsonObject payload;
        payload["success"] = false;
        payload["message"] = QString::fromStdString(reason);

        return Message(Command::LoginResult, payload);
    }

}
