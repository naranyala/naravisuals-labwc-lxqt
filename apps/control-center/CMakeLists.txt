cmake_minimum_required(VERSION 3.16)
project(nv-control-center LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(nv-control-center main.cpp)

target_link_libraries(nv-control-center PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

install(TARGETS nv-control-center
    RUNTIME DESTINATION bin
)
