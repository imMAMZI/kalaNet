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
        protocol/buy_message.cpp

        utils/hash_utils.cpp
        utils/id_generator.cpp
        utils/time_utils.cpp
)
find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network
        REQUIRED
)
target_include_directories(common PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(common PROPERTIES
        CXX_STANDARD 23
        CXX_STANDARD_REQUIRED ON
)
target_link_libraries(common
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
)
