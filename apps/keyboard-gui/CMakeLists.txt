cmake_minimum_required(VERSION 3.16)
project(nv-keyboard-gui LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
add_executable(nv-keyboard-gui main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/healthcheck.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/healthcheckwidget.cpp
)
target_include_directories(nv-keyboard-gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../shared)
target_link_libraries(nv-keyboard-gui PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
install(TARGETS nv-keyboard-gui RUNTIME DESTINATION bin)
