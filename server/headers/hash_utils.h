#ifndef HASH_UTILS_H
#define HASH_UTILS_H

#include <string>

namespace common::utils {

    /**
     * @brief Hashes a plain text password using SHA-256
     * @param plainPassword Raw password (never store this)
     * @return SHA-256 hash as hex string
     */
    std::string hashPassword(const std::string& plainPassword);

    /**
     * @brief Verifies a password against a stored hash
     * @param plainPassword Input password
     * @param storedHash Previously stored hash
     * @return true if matches, false otherwise
     */
    bool verifyPassword(const std::string& plainPassword,
                        const std::string& storedHash);

}

#endif // HASH_UTILS_H
