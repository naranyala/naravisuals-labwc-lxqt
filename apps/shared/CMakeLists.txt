cmake_minimum_required(VERSION 3.16)
project(nv-healthcheck LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(nv-healthcheck
    healthcheck-cli.cpp
    healthcheck.cpp
)

target_include_directories(nv-healthcheck PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(nv-healthcheck PRIVATE Qt6::Core)
