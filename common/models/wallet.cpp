#include "models/wallet.h"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace common {

    static std::string currentTimestamp() {
        using namespace std::chrono;

        auto now = system_clock::now();
        std::time_t t = system_clock::to_time_t(now);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    Wallet::Wallet()
        : balance_(0.0)
    {
    }

    double Wallet::balance() const {
        return balance_;
    }

    void Wallet::deposit(double amount) {
        if (amount <= 0)
            return;

        balance_ += amount;

        std::stringstream ss;
        ss << "[" << currentTimestamp() << "] "
           << "Deposit +" << amount << " tokens";
        addHistory(ss.str());
    }

    bool Wallet::withdraw(double amount) {
        if (amount <= 0 || amount > balance_)
            return false;

        balance_ -= amount;

        std::stringstream ss;
        ss << "[" << currentTimestamp() << "] "
           << "Withdraw -" << amount << " tokens";
        addHistory(ss.str());

        return true;
    }

    const std::vector<std::string>& Wallet::history() const {
        return history_;
    }

    void Wallet::addHistory(const std::string& record) {
        history_.push_back(record);
    }

}
