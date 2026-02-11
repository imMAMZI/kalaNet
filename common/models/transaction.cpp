#include "models/transaction.h"
#include <iomanip>
#include <sstream>

namespace common {

    // Utility to generate ISO timestamp (YYYY-MM-DD HH:MM:SS)
    static std::string currentTimestamp() {
        using namespace std::chrono;

        auto now = system_clock::now();
        std::time_t t = system_clock::to_time_t(now);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    Transaction::Transaction(
            int id,
            const std::string& buyerUsername,
            const std::string& sellerUsername,
            int adId,
            double amount,
            TransactionStatus status)
        : id_(id),
          buyer_(buyerUsername),
          seller_(sellerUsername),
          adId_(adId),
          amount_(amount),
          status_(status),
          timestamp_(currentTimestamp())
    {
    }

    int Transaction::id() const { return id_; }

    const std::string& Transaction::buyer() const { return buyer_; }

    const std::string& Transaction::seller() const { return seller_; }

    int Transaction::adId() const { return adId_; }

    double Transaction::amount() const { return amount_; }

    TransactionStatus Transaction::status() const { return status_; }

    std::string Transaction::timestamp() const { return timestamp_; }

    void Transaction::setStatus(TransactionStatus newStatus) {
        status_ = newStatus;
    }

}
