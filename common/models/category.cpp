#include "models/category.h"

namespace common {

    Category::Category(int id, const std::string& name)
        : id_(id), name_(name)
    {
    }

    int Category::id() const {
        return id_;
    }

    const std::string& Category::name() const {
        return name_;
    }

}
