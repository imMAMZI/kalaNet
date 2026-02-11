#include "models/ad.h"

namespace common {

    Ad::Ad(
        int id,
        const std::string& title,
        const std::string& description,
        double price,
        const Category& category,
        const std::string& sellerUsername,
        const std::string& imagePath)
        : id_(id),
          title_(title),
          description_(description),
          price_(price),
          category_(category),
          seller_(sellerUsername),
          imagePath_(imagePath),
          status_(AdStatus::Pending)   // PDF: default is Pending
    {
    }

    int Ad::id() const { return id_; }

    const std::string& Ad::title() const { return title_; }

    const std::string& Ad::description() const { return description_; }

    double Ad::price() const { return price_; }

    const Category& Ad::category() const { return category_; }

    const std::string& Ad::seller() const { return seller_; }

    const std::string& Ad::imagePath() const { return imagePath_; }

    AdStatus Ad::status() const { return status_; }

    void Ad::approve() {
        status_ = AdStatus::Approved;
    }

    void Ad::markSold() {
        status_ = AdStatus::Sold;
    }

}
