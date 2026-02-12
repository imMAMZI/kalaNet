cmake_minimum_required(VERSION 4.0)
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
        REQUIRED
)

add_executable(serverProject
        main.cpp
        auth/AuthService.cpp
        auth/AuthService.h
        network/ClientConnection.cpp
        network/ClientConnection.h
        network/TcpServer.cpp
        network/TcpServer.h
        protocol/RequestDispatcher.cpp
        protocol/RequestDispatcher.h
        repository/SqliteUserRepository.cpp
        repository/SqliteUserRepository.h
        repository/UserRepository.h
        security/PasswordHasher.cpp
        security/PasswordHasher.h

)

target_link_libraries(serverProject
        PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        common
)
