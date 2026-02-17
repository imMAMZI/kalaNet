#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <chrono>

namespace common {

    enum class TransactionStatus {
        Pending,
        Success,
        Failed
    };

    class Transaction {
    public:
        Transaction() = default;

        Transaction(
            int id,
            const std::string& buyerUsername,
            const std::string& sellerUsername,
            int adId,
            double amount,
            TransactionStatus status
        );

        int id() const;
        const std::string& buyer() const;
        const std::string& seller() const;
        int adId() const;
        double amount() const;
        TransactionStatus status() const;
        std::string timestamp() const;

        void setStatus(TransactionStatus newStatus);

    private:
        int id_;
        std::string buyer_;
        std::string seller_;
        int adId_;
        double amount_;
        TransactionStatus status_;
        std::string timestamp_;
    };

}

#endif // TRANSACTION_H
