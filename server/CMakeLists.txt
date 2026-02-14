cmake_minimum_required(VERSION 3.21)
project(serverProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network
        Sql
        REQUIRED
)

add_executable(serverProject
        main.cpp
        ads/ad_service.cpp
        ads/ad_service.h
        auth/auth_service.cpp
        auth/auth_service.h
        network/client_connection.cpp
        network/client_connection.h
        network/tcp_server.cpp
        network/tcp_server.h
        protocol/request_dispatcher.cpp
        protocol/request_dispatcher.h
        repository/sqlite_user_repository.cpp
        repository/sqlite_user_repository.h
        repository/user_repository.h
        security/password_hasher.cpp
        security/password_hasher.h
        ui/request_log_model.h
        ui/request_log_model.cpp
        ui/server_console_window.h
        ui/server_console_window.cpp
        ui/request_log_filter_proxy.h
        ui/request_log_filter_proxy.cpp
        ui/pending_ads_window.h
        ui/pending_ads_window.cpp
        ui/pending_ads_window.ui

)

target_link_libraries(serverProject
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Sql
        common
)
