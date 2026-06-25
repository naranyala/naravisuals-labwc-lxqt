cmake_minimum_required(VERSION 3.16)
project(naravisuals-weather-widget LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(COMPILE_LXQT_PANEL ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(lxqt-panel REQUIRED)

add_library(naravisuals-weather-widget SHARED
    naravisuals-weather-widget.cpp
    naravisuals-weather-widget.h
)

target_include_directories(naravisuals-weather-widget PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${lxqt-panel_INCLUDE_DIRS}
)

target_link_libraries(naravisuals-weather-widget PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network
    ${lxqt-panel_LIBRARIES}
)

install(TARGETS naravisuals-weather-widget
    LIBRARY DESTINATION ${lxqt-panel_PLUGINS_DIR}
)

install(FILES naravisuals-weather-widget.desktop
    DESTINATION ${lxqt-panel_SHARE_DIR}
)
