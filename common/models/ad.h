#ifndef AD_H
#define AD_H

#include <string>
#include "models/category.h"

namespace common {

    enum class AdStatus {
        Pending,
        Approved,
        Sold
    };

    class Ad {
    public:
        Ad() = default;

        Ad(
            int id,
            const std::string& title,
            const std::string& description,
            double price,
            const Category& category,
            const std::string& sellerUsername,
            const std::string& imagePath
        );

        int id() const;
        const std::string& title() const;
        const std::string& description() const;
        double price() const;
        const Category& category() const;
        const std::string& seller() const;
        const std::string& imagePath() const;
        AdStatus status() const;

        void approve();   // Admin only
        void markSold();  // After successful transaction

    private:
        int id_;
        std::string title_;
        std::string description_;
        double price_;
        Category category_;
        std::string seller_; // intentional
        std::string imagePath_;
        AdStatus status_;
    };

}

#endif // AD_H
