#ifndef SIGNUP_MESSAGE_H
#define SIGNUP_MESSAGE_H

#include "protocol/message.h"

namespace common {

    class SignupMessage {
    public:
        static Message createRequest(
            const std::string& fullName,
            const std::string& username,
            const std::string& phone,
            const std::string& email,
            const std::string& password
        );

        static Message createSuccessResponse(
            const std::string& message = "Signup Was Successful"
        );

        static Message createFailureResponse(
            const std::string& reason
        );
    };

}

#endif // SIGNUP_MESSAGE_H
