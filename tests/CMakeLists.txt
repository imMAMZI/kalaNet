cmake_minimum_required(VERSION 3.21)

find_package(Qt6 COMPONENTS Core Test Sql REQUIRED)

add_executable(kalanet_tests
    test_services.cpp
    ../server/auth/auth_service.cpp
    ../server/ads/ad_service.cpp
    ../server/cart/cart_service.cpp
    ../server/wallet/wallet_service.cpp
    ../server/security/password_hasher.cpp
    ../server/repository/sqlite_wallet_repository.cpp
    ../server/repository/sqlite_ad_repository.cpp
    ../server/repository/sqlite_cart_repository.cpp
    ../server/repository/sqlite_user_repository.cpp
    ../server/logging_audit_logger.cpp
)

target_include_directories(kalanet_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/server
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(kalanet_tests PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Sql
    common
)

add_test(NAME kalanet_tests COMMAND kalanet_tests)
