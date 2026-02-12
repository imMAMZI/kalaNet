#ifndef LOGIN_MESSAGE_H
#define LOGIN_MESSAGE_H

#include "protocol/message.h"

namespace common {

    class LoginMessage {
    public:
        static Message createRequest(
            const std::string& username,
            const std::string& password
        );

        static Message createSuccessResponse(
            const std::string& fullName,
            const std::string& role
        );

        static Message createFailureResponse(
            const std::string& reason
        );
    };

}

#endif // LOGIN_MESSAGE_H
