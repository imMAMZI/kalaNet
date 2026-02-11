#ifndef CATEGORY_H
#define CATEGORY_H

#include <string>

namespace common {

    class Category {
    public:
        Category() = default;
        Category(int id, const std::string& name);

        int id() const;
        const std::string& name() const;

    private:
        int id_;
        std::string name_;
    };

}

#endif // CATEGORY_H
