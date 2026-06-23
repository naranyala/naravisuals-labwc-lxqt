cmake_minimum_required(VERSION 3.16)
project(nv-network-gui LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
add_executable(nv-network-gui main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/healthcheck.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/healthcheckwidget.cpp
)
target_include_directories(nv-network-gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../shared)
target_link_libraries(nv-network-gui PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

install(TARGETS nv-network-gui
    RUNTIME DESTINATION bin
)
