#ifndef CART_H
#define CART_H

#include <vector>

namespace common {

    class Cart {
    public:
        Cart() = default;

        bool addItem(int adId);

        bool removeItem(int adId);

        void clear();

        const std::vector<int>& items() const;

        bool contains(int adId) const;

    private:
        std::vector<int> items_;
    };

}

#endif // CART_H
