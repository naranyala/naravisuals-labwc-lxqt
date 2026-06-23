cmake_minimum_required(VERSION 3.16)
project(nv-screenshot-gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(nv-screenshot-gui main.cpp)
target_link_libraries(nv-screenshot-gui PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

install(TARGETS nv-screenshot-gui
    RUNTIME DESTINATION bin
)
