#include "models/cart.h"
#include <algorithm>

namespace common {

    bool Cart::addItem(int adId) {
        if (contains(adId))
            return false; // already added

        items_.push_back(adId);
        return true;
    }

    bool Cart::removeItem(int adId) {
        auto it = std::find(items_.begin(), items_.end(), adId);
        if (it == items_.end())
            return false;

        items_.erase(it);
        return true;
    }

    void Cart::clear() {
        items_.clear();
    }

    const std::vector<int>& Cart::items() const {
        return items_;
    }

    bool Cart::contains(int adId) const {
        return std::find(items_.begin(), items_.end(), adId) != items_.end();
    }

}
