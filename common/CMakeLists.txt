cmake_minimum_required(VERSION 4.0)

add_library(common STATIC
        models/user.cpp
        models/ad.cpp
        models/wallet.cpp
        models/cart.cpp
        models/transaction.cpp
        models/category.cpp

        protocol/message.cpp
        protocol/serializer.cpp

        utils/hash_utils.cpp
        utils/id_generator.cpp
        utils/time_utils.cpp
)

target_include_directories(common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(common PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
)