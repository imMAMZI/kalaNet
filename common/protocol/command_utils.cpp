#include "command_utils.h"

using namespace common;

QString common::commandToString(Command cmd)
{
    switch (cmd) {
    case Command::Login:        return "Login";
    case Command::Signup:       return "Signup";
    case Command::LoginResult:  return "LoginResult";
    case Command::SignupResult: return "SignupResult";
    case Command::Error:        return "Error";
    case Command::GetAds:       return "GetAds";
    case Command::AddToCart:    return "AddToCart";
    case Command::RemoveFromCart:return "RemoveFromCart";
    case Command::Buy:          return "Buy";
    case Command::BuyResult:    return "BuyResult";
    }
    return "Error";
}

Command common::stringToCommand(const QString& str)
{
    if (str == "Login") return Command::Login;
    if (str == "Signup") return Command::Signup;
    if (str == "LoginResult") return Command::LoginResult;
    if (str == "SignupResult") return Command::SignupResult;
    if (str == "Error") return Command::Error;
    if (str == "GetAds") return Command::GetAds;
    if (str == "AddToCart") return Command::AddToCart;
    if (str == "RemoveFromCart") return Command::RemoveFromCart;
    if (str == "Buy") return Command::Buy;
    if (str == "BuyResult") return Command::BuyResult;

    return Command::Error;
}
