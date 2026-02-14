#include "session_service.h"

#include <QUuid>
#include <QMutexLocker>

QString SessionService::createSession(const QString& username, const QString& role)
{
    const QString normalizedUsername = username.trimmed();
    if (normalizedUsername.isEmpty()) {
        return {};
    }

    const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QMutexLocker locker(&mutex_);
    sessionsByToken_.insert(token, SessionInfo{normalizedUsername, role.trimmed()});
    return token;
}

bool SessionService::validateSession(const QString& token, SessionInfo* outSession) const
{
    const QString normalizedToken = token.trimmed();
    if (normalizedToken.isEmpty()) {
        return false;
    }

    QMutexLocker locker(&mutex_);
    const auto it = sessionsByToken_.constFind(normalizedToken);
    if (it == sessionsByToken_.constEnd()) {
        return false;
    }

    if (outSession) {
        *outSession = it.value();
    }
    return true;
}

bool SessionService::invalidateSession(const QString& token)
{
    const QString normalizedToken = token.trimmed();
    if (normalizedToken.isEmpty()) {
        return false;
    }

    QMutexLocker locker(&mutex_);
    return sessionsByToken_.remove(normalizedToken) > 0;
}

QString SessionService::refreshSession(const QString& token)
{
    const QString normalizedToken = token.trimmed();
    if (normalizedToken.isEmpty()) {
        return {};
    }

    QMutexLocker locker(&mutex_);
    const auto it = sessionsByToken_.find(normalizedToken);
    if (it == sessionsByToken_.end()) {
        return {};
    }

    const SessionInfo session = it.value();
    sessionsByToken_.erase(it);

    const QString newToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
    sessionsByToken_.insert(newToken, session);
    return newToken;
}

bool SessionService::updateSessionUsername(const QString& token, const QString& newUsername)
{
    const QString normalizedToken = token.trimmed();
    const QString normalizedUsername = newUsername.trimmed();
    if (normalizedToken.isEmpty() || normalizedUsername.isEmpty()) {
        return false;
    }

    QMutexLocker locker(&mutex_);
    const auto it = sessionsByToken_.find(normalizedToken);
    if (it == sessionsByToken_.end()) {
        return false;
    }

    it->username = normalizedUsername;
    return true;
}
