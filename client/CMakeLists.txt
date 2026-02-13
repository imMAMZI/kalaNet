cmake_minimum_required(VERSION 4.0)
project(clientProject)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network
        REQUIRED)


add_executable(clientProject src/client_main.cpp
        main_window/client_main_window.cpp
        main_window/client_main_window.h
        main_window/client_main_window.ui
        login/login_window.cpp
        login/login_window.h
        login/login_window.ui
        signup/signup_window.cpp
        signup/signup_window.h
        signup/signup_window.ui
        network/auth_client.cpp
        network/auth_client.h)

target_link_libraries(clientProject
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        common
)
