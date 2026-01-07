cmake_minimum_required(VERSION 4.0)
project(finalProject)

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

add_executable(finalProject src/main.cpp
        src/mymainwindow.cpp
        headers/mymainwindow.h
        src/mymainwindow.ui)
target_link_libraries(finalProject
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
)

