cmake_minimum_required(VERSION 3.16)
project(nv-power-gui LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Xml)
add_executable(nv-power-gui main.cpp)
target_link_libraries(nv-power-gui PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Xml)

install(TARGETS nv-power-gui
    RUNTIME DESTINATION bin
)
