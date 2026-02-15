#ifndef KALANET_AUTH_CLIENT_H
#define KALANET_AUTH_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>
#include <QQueue>
#include <QJsonArray>
#include <QJsonObject>

#include "protocol/message.h"

class AuthClient : public QObject
{
    Q_OBJECT

public:
    static AuthClient* instance();

    void sendMessage(const common::Message& message);
    void connectIfNeeded();
    bool isConnected() const;

    QString sessionToken() const;
    QString username() const;
    QString fullName() const;

    common::Message withSession(common::Command command, const QJsonObject& payload = {}, const QString& requestId = {}) const;

signals:
    void loginResultReceived(bool success,
                             const QString& message,
                             const QString& fullName,
                             const QString& role);

    void signupResultReceived(bool success,
                              const QString& message);

    void adCreateResultReceived(bool success,
                                const QString& message,
                                int adId);

    void adListReceived(bool success,
                        const QString& message,
                        const QJsonArray& ads);

    void adDetailResultReceived(bool success,
                                const QString& message,
                                const QJsonObject& ad);

    void cartListReceived(bool success,
                          const QString& message,
                          const QJsonArray& items);

    void cartAddItemResultReceived(bool success,
                                   const QString& message,
                                   int adId,
                                   bool added);

    void cartRemoveItemResultReceived(bool success,
                                      const QString& message,
                                      int adId);

    void cartClearResultReceived(bool success,
                                 const QString& message);

    void walletBalanceReceived(bool success,
                               const QString& message,
                               int balanceTokens);

    void walletTopUpResultReceived(bool success,
                                   const QString& message,
                                   int balanceTokens);

    void buyResultReceived(bool success,
                           const QString& message,
                           int balanceTokens,
                           const QJsonArray& soldAdIds);

    void profileHistoryReceived(bool success,
                                const QString& message,
                                const QJsonObject& payload);

    void profileUpdateResultReceived(bool success,
                                     const QString& message,
                                     const QJsonObject& payload);

    void captchaChallengeReceived(bool success,
                                  const QString& message,
                                  const QString& scope,
                                  const QString& challengeText,
                                  const QString& nonce,
                                  const QString& expiresAt);

    void adStatusNotifyReceived(const QJsonArray& soldAdIds,
                                const QString& status);

    void networkError(const QString& message);

private:
    explicit AuthClient(QObject* parent = nullptr);

    void sendFramed(const common::Message& message);
    static bool messageSuccess(const common::Message& message);

private slots:
    void onConnected();
    void onReadyRead();

private:
    QTcpSocket socket_;
    QByteArray buffer_;
    qint32 expectedSize_ = -1;
    QQueue<common::Message> pendingMessages_;

    QString sessionToken_;
    QString username_;
    QString fullName_;

    static constexpr quint16 kPort = 8080;
    static constexpr const char* kHost = "127.0.0.1";
};

#endif // KALANET_AUTH_CLIENT_H
