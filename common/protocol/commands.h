#ifndef COMMANDS_H
#define COMMANDS_H

namespace common::protocol {

    enum class Command {
        Login,
        Signup,
        LoginResult,
        SignupResult,
        Error
    };

}

#endif // COMMANDS_H
