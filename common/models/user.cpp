#include "models/user.h"

namespace common {

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
          role_(UserRole::User)
    {
    }

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

    void User::setFullName(const std::string& name) {
        fullName_ = name;
    }

    void User::setEmail(const std::string& email) {
        email_ = email;
    }

    void User::setPhone(const std::string& phone) {
        phone_ = phone;
    }

    const std::string& User::getPasswordHash() const {
        return passwordHash_;
    }

    void User::setPasswordHash(const std::string& hash) {
        passwordHash_ = hash;
    }

    Wallet& User::wallet()
    {
        return wallet_;
    }
    const Wallet& User::wallet() const
    {
        return wallet_;
    }

    UserRole User::getRole() const {
        return role_;
    }

    bool User::isAdmin() const {
        return role_ == UserRole::Admin;
    }

    const std::vector<std::string>& User::getHistory() const {
        return history_;
    }

    void User::addHistoryRecord(const std::string& record) {
        history_.push_back(record);
    }

    Cart& User::cart()
    {
        return cart_;
    }
    const Cart& User::cart() const
    {
        return cart_;
    }

    void User::setRole(UserRole role)
    {
        role_ = role;
    }
}