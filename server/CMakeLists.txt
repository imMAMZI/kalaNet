cmake_minimum_required(VERSION 4.0)
project(serverProject)

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

add_executable(serverProject src/server_main.cpp
        src/mymainwindow.cpp
        headers/mymainwindow.h
        src/mymainwindow.ui
        )

target_link_libraries(serverProject
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
)


