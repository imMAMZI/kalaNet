#include "password_hasher.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QStringList>

namespace {
constexpr auto kScheme = "knet_sha256";
constexpr int kDefaultIterations = 120000;
constexpr int kSaltLengthBytes = 16;

QByteArray generateSalt()
{
    QByteArray salt(kSaltLengthBytes, Qt::Uninitialized);
    for (int i = 0; i < salt.size(); ++i) {
        salt[i] = static_cast<char>(QRandomGenerator::system()->bounded(256));
    }
    return salt;
}

QByteArray sha256(const QByteArray& input)
{
    return QCryptographicHash::hash(input, QCryptographicHash::Sha256);
}
}

QString PasswordHasher::hash(const QString& rawPassword)
{
    const QByteArray salt = generateSalt();
    const QString digestHex = deriveHash(rawPassword, salt, kDefaultIterations);

    return QStringLiteral("%1$%2$%3$%4")
        .arg(QString::fromLatin1(kScheme))
        .arg(kDefaultIterations)
        .arg(QString::fromLatin1(salt.toHex()))
        .arg(digestHex);
}

bool PasswordHasher::verify(const QString& rawPassword, const QString& storedHash)
{
    const QStringList parts = storedHash.split('$');
    if (parts.size() == 4 && parts[0] == QLatin1String(kScheme)) {
        bool ok = false;
        const int iterations = parts[1].toInt(&ok);
        if (!ok || iterations <= 0) {
            return false;
        }

        const QByteArray salt = QByteArray::fromHex(parts[2].toLatin1());
        const QByteArray expected = QByteArray::fromHex(parts[3].toLatin1());
        if (salt.isEmpty() || expected.isEmpty()) {
            return false;
        }

        const QByteArray actual = QByteArray::fromHex(
            deriveHash(rawPassword, salt, iterations).toLatin1());
        return constantTimeEquals(actual, expected);
    }

    const QByteArray storedBytes = storedHash.toUtf8();
    const QByteArray rawBytes = rawPassword.toUtf8();
    const QByteArray rawSha256Hex = sha256(rawBytes).toHex();

    return constantTimeEquals(storedBytes, rawBytes) ||
           constantTimeEquals(storedBytes, rawSha256Hex);
}

QString PasswordHasher::deriveHash(const QString& rawPassword,
                                   const QByteArray& salt,
                                   int iterations)
{
    const QByteArray passwordBytes = rawPassword.toUtf8();

    QByteArray digest = sha256(salt + passwordBytes);
    for (int i = 1; i < iterations; ++i) {
        digest = sha256(digest + salt + passwordBytes);
    }

    return QString::fromLatin1(digest.toHex());
}

bool PasswordHasher::constantTimeEquals(const QByteArray& lhs,
                                        const QByteArray& rhs)
{
    if (lhs.size() != rhs.size()) {
        return false;
    }

    uchar diff = 0;
    for (int i = 0; i < lhs.size(); ++i) {
        diff |= static_cast<uchar>(lhs[i] ^ rhs[i]);
    }
    return diff == 0;
}
