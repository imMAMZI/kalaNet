#ifndef PASSWORD_HASHER_H
#define PASSWORD_HASHER_H

#include <QString>

class PasswordHasher
{
public:
    static QString hash(const QString& rawPassword);
};

#endif // PASSWORD_HASHER_H
