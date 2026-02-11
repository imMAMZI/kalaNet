#ifndef CART_H
#define CART_H

#include <vector>

namespace common {

    class Ad; // forward declaration

    class Cart {
    public:
        Cart() = default;

        // Try to add an Ad by ID – validated on server side
        bool addItem(int adId);

        // Remove Ad ID from cart
        bool removeItem(int adId);

        // Clear all items
        void clear();

        // Get list of Ad IDs
        const std::vector<int>& items() const;

        // Check if an Ad ID already exists in cart
        bool contains(int adId) const;

    private:
        std::vector<int> items_;
    };

}

#endif // CART_H
