#include "models/user.h"

namespace common {

    // Constructor
    User::User(std::string username,
               std::string fullName,
               std::string email,
               std::string phone,
               std::string passwordHash)
        : username_(std::move(username)),
          fullName_(std::move(fullName)),
          email_(std::move(email)),
          phone_(std::move(phone)),
          passwordHash_(std::move(passwordHash)),
          walletBalance_(0),
          role_(UserRole::User)
    {
    }

    // Identity getters
    const std::string& User::getUsername() const {
        return username_;
    }

    const std::string& User::getFullName() const {
        return fullName_;
    }

    const std::string& User::getEmail() const {
        return email_;
    }

    const std::string& User::getPhone() const {
        return phone_;
    }

    // Identity setters
    void User::setFullName(const std::string& name) {
        fullName_ = name;
    }

    void User::setEmail(const std::string& email) {
        email_ = email;
    }

    void User::setPhone(const std::string& phone) {
        phone_ = phone;
    }

    // Authentication
    const std::string& User::getPasswordHash() const {
        return passwordHash_;
    }

    void User::setPasswordHash(const std::string& hash) {
        passwordHash_ = hash;
    }

    // Wallet
    std::int64_t User::getBalance() const {
        return walletBalance_;
    }

    void User::addBalance(std::int64_t amount) {
        if (amount > 0) {
            walletBalance_ += amount;
        }
    }

    bool User::deductBalance(std::int64_t amount) {
        if (amount <= 0) {
            return false;
        }

        if (walletBalance_ < amount) {
            return false;
        }

        walletBalance_ -= amount;
        return true;
    }

    // Role
    UserRole User::getRole() const {
        return role_;
    }

    bool User::isAdmin() const {
        return role_ == UserRole::Admin;
    }

    // History
    const std::vector<std::string>& User::getHistory() const {
        return history_;
    }

    void User::addHistoryRecord(const std::string& record) {
        history_.push_back(record);
    }

} // namespace common
