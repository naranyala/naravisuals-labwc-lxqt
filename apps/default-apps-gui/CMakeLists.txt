cmake_minimum_required(VERSION 3.16)
project(nv-default-apps-gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(nv-default-apps-gui main.cpp)

target_link_libraries(nv-default-apps-gui PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

install(TARGETS nv-default-apps-gui
    RUNTIME DESTINATION bin
)
