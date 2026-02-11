#ifndef COMMANDS_H
#define COMMANDS_H

namespace common
{
    enum class Command
    {
        Login,
        Signup,
        LoginResult,
        SignupResult,
        Error,
        GetAds,
        AddToCart,
        RemoveFromCart,
        Buy, // Client → Server
        BuyResult, // Server → Client
    };
}

#endif // COMMANDS_H
