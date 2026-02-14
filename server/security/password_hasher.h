#ifndef PASSWORD_HASHER_H
#define PASSWORD_HASHER_H

#include <QString>

class PasswordHasher
{
public:
    static QString hash(const QString& rawPassword);
    static bool verify(const QString& rawPassword, const QString& storedHash);

private:
    static QString deriveHash(const QString& rawPassword,
                              const QByteArray& salt,
                              int iterations);
    static bool constantTimeEquals(const QByteArray& lhs,
                                   const QByteArray& rhs);
};

#endif // PASSWORD_HASHER_H
