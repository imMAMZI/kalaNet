cmake_minimum_required(VERSION 3.21)

project(KalaNet LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)

