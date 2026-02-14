#include "captcha_service.h"

#include <QMutexLocker>
#include <QRandomGenerator>
#include <QUuid>

namespace {
constexpr int kMinOperand = 1;
constexpr int kMaxOperandExclusive = 25;
constexpr int kTtlSeconds = 120;
}

CaptchaService::Challenge CaptchaService::createChallenge(const QString& scope)
{
    const int left = QRandomGenerator::global()->bounded(kMinOperand, kMaxOperandExclusive);
    const int right = QRandomGenerator::global()->bounded(kMinOperand, kMaxOperandExclusive);

    Challenge challenge;
    challenge.nonce = QUuid::createUuid().toString(QUuid::WithoutBraces);
    challenge.scope = scope.trimmed().toLower();
    challenge.challengeText = QStringLiteral("%1 + %2 = ?").arg(left).arg(right);
    challenge.expiresAt = QDateTime::currentDateTimeUtc().addSecs(kTtlSeconds);

    QMutexLocker locker(&mutex_);
    cleanupExpiredLocked(QDateTime::currentDateTimeUtc());
    challenges_.insert(challenge.nonce,
                       CaptchaEntry{left + right, challenge.scope, challenge.expiresAt});

    return challenge;
}

bool CaptchaService::verifyAndConsume(const QString& nonce,
                                      int answer,
                                      const QString& scope,
                                      QString* failureReason)
{
    const QString normalizedNonce = nonce.trimmed();
    const QString normalizedScope = scope.trimmed().toLower();
    const QDateTime now = QDateTime::currentDateTimeUtc();

    QMutexLocker locker(&mutex_);
    cleanupExpiredLocked(now);

    auto it = challenges_.find(normalizedNonce);
    if (it == challenges_.end()) {
        if (failureReason) {
            *failureReason = QStringLiteral("CAPTCHA challenge is missing or expired");
        }
        return false;
    }

    const CaptchaEntry entry = it.value();
    challenges_.erase(it);

    if (entry.scope != normalizedScope) {
        if (failureReason) {
            *failureReason = QStringLiteral("CAPTCHA scope is invalid");
        }
        return false;
    }

    if (entry.expiresAt < now) {
        if (failureReason) {
            *failureReason = QStringLiteral("CAPTCHA challenge expired");
        }
        return false;
    }

    if (entry.answer != answer) {
        if (failureReason) {
            *failureReason = QStringLiteral("CAPTCHA verification failed");
        }
        return false;
    }

    return true;
}

void CaptchaService::cleanupExpiredLocked(const QDateTime& now)
{
    for (auto it = challenges_.begin(); it != challenges_.end();) {
        if (it.value().expiresAt < now) {
            it = challenges_.erase(it);
        } else {
            ++it;
        }
    }
}
