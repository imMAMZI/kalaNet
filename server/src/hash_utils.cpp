#include "../headers/hash_utils.h"

#include <QCryptographicHash>
#include <QString>
#include <QByteArray>

namespace common::utils {

    std::string hashPassword(const std::string& plainPassword) {
        QByteArray passwordBytes = QByteArray::fromStdString(plainPassword);

        QByteArray hashBytes = QCryptographicHash::hash(
            passwordBytes,
            QCryptographicHash::Sha256
        );

        return hashBytes.toHex().toStdString();
    }

    bool verifyPassword(const std::string& plainPassword,
                        const std::string& storedHash) {
        return hashPassword(plainPassword) == storedHash;
    }

}
