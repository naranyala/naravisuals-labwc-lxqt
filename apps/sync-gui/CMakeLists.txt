cmake_minimum_required(VERSION 3.16)
project(nv-sync-gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

add_executable(nv-sync-gui main.cpp)
target_link_libraries(nv-sync-gui PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

# Testing Suite
enable_testing()
add_executable(test-sync-gui tests/test_sync.cpp)
target_link_libraries(test-sync-gui PRIVATE Qt6::Core Qt6::Test)
add_test(NAME SyncGuiTests COMMAND test-sync-gui)

install(TARGETS nv-sync-gui
    RUNTIME DESTINATION bin
)
