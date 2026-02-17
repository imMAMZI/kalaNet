#ifndef WALLET_H
#define WALLET_H

#include <vector>
#include <string>

namespace common {

    class Wallet {
    public:
        Wallet();

        double balance() const;

        void deposit(double amount);

        bool withdraw(double amount);

        const std::vector<std::string>& history() const;

    private:
        double balance_;
        std::vector<std::string> history_;

        void addHistory(const std::string& record);
    };

}

#endif // WALLET_H
