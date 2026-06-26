cmake_minimum_required(VERSION 3.16)
project(nv-panel-gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(nv-panel-gui main.cpp)

target_compile_definitions(nv-panel-gui PRIVATE
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}/../.."
)

target_link_libraries(nv-panel-gui PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

install(TARGETS nv-panel-gui
    RUNTIME DESTINATION bin
)
