cmake_minimum_required(VERSION 3.16)
project(nv-env-gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

add_executable(nv-env-gui main.cpp mainwindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/healthcheck.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared/healthcheckwidget.cpp
)
target_include_directories(nv-env-gui PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../shared)
target_link_libraries(nv-env-gui PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

enable_testing()
add_subdirectory(tests)

install(TARGETS nv-env-gui
    RUNTIME DESTINATION bin
)
