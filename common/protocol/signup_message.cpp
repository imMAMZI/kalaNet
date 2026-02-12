#include "protocol/signup_message.h"
#include "protocol/commands.h"
#include <QJsonObject>
#include <QString>

namespace common {

    Message SignupMessage::createRequest(
        const std::string& fullName,
        const std::string& username,
        const std::string& phone,
        const std::string& email,
        const std::string& password)
    {
        QJsonObject payload;
        payload["fullName"] = QString::fromStdString(fullName);
        payload["username"] = QString::fromStdString(username);
        payload["phone"] = QString::fromStdString(phone);
        payload["email"] = QString::fromStdString(email);
        payload["password"] = QString::fromStdString(password);

        return Message(Command::Signup, payload);
    }

    Message SignupMessage::createSuccessResponse(
        const std::string& message)
    {
        QJsonObject payload;
        payload["success"] = true;
        payload["message"] = QString::fromStdString(message);

        return Message(Command::SignupResult, payload);
    }

    Message SignupMessage::createFailureResponse(
        const std::string& reason)
    {
        QJsonObject payload;
        payload["success"] = false;
        payload["message"] = QString::fromStdString(reason);

        return Message(Command::SignupResult, payload);
    }

}
