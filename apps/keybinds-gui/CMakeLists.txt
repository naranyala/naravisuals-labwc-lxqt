cmake_minimum_required(VERSION 3.16)
project(nv-keybinds-gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Xml)

add_executable(nv-keybinds-gui main.cpp)

target_link_libraries(nv-keybinds-gui PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Xml)

install(TARGETS nv-keybinds-gui
    RUNTIME DESTINATION bin
)
