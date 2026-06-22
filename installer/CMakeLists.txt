cmake_minimum_required(VERSION 3.16)
project(NaraVisualsThemeManager VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt6 components
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)

add_executable(NaraVisualsThemeManager
    main.cpp
)

target_link_libraries(NaraVisualsThemeManager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
