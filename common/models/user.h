#ifndef USER_H
#define USER_H

#include <string>
#include <vector>
#include <cstdint>

#include "models/cart.h"
#include "models/wallet.h"

namespace common {

    enum class UserRole {
        User,
        Admin
    };

    class User {
    public:
        // Constructors
        User() = default;
        User(std::string username,
             std::string fullName,
             std::string email,
             std::string phone,
             std::string passwordHash);


        const std::string& getUsername() const;
        const std::string& getFullName() const;
        const std::string& getEmail() const;
        const std::string& getPhone() const;

        void setFullName(const std::string& name);
        void setEmail(const std::string& email);
        void setPhone(const std::string& phone);

        const std::string& getPasswordHash() const;
        void setPasswordHash(const std::string& hash);


        Wallet& wallet();
        const Wallet& wallet() const;


        Cart& cart();
        const Cart& cart() const;


        UserRole getRole() const;
        bool isAdmin() const;

        const std::vector<std::string>& getHistory() const;
        void addHistoryRecord(const std::string& record);

        void setRole(UserRole role);

    private:
        std::string username_;
        std::string fullName_;
        std::string email_;
        std::string phone_;

        Wallet wallet_;
        Cart cart_;

        std::string passwordHash_;

        UserRole role_ {UserRole::User};

        std::vector<std::string> history_;
    };

}

#endif // USER_H
