#ifndef KALANET_SESSION_SERVICE_H
#define KALANET_SESSION_SERVICE_H

#include <QString>
#include <QHash>
#include <QMutex>

class SessionService
{
public:
    struct SessionInfo {
        QString username;
        QString role;
    };

    QString createSession(const QString& username, const QString& role);
    bool validateSession(const QString& token, SessionInfo* outSession = nullptr) const;
    bool invalidateSession(const QString& token);
    QString refreshSession(const QString& token);
    bool updateSessionUsername(const QString& token, const QString& newUsername);

private:
    mutable QMutex mutex_;
    QHash<QString, SessionInfo> sessionsByToken_;
};

#endif // KALANET_SESSION_SERVICE_H
