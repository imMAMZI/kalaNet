#ifndef CAPTCHA_SERVICE_H
#define CAPTCHA_SERVICE_H

#include <QDateTime>
#include <QHash>
#include <QMutex>
#include <QString>

class CaptchaService
{
public:
    struct Challenge {
        QString nonce;
        QString challengeText;
        QString scope;
        QDateTime expiresAt;
    };

    Challenge createChallenge(const QString& scope);
    bool verifyAndConsume(const QString& nonce,
                          int answer,
                          const QString& scope,
                          QString* failureReason = nullptr);

private:
    struct CaptchaEntry {
        int answer = 0;
        QString scope;
        QDateTime expiresAt;
    };

    void cleanupExpiredLocked(const QDateTime& now);

    QHash<QString, CaptchaEntry> challenges_;
    QMutex mutex_;
};

#endif // CAPTCHA_SERVICE_H
