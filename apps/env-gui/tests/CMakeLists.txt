set(CMAKE_AUTOMOC ON)
add_executable(test_env_gui test_env_gui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/healthcheck.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/healthcheckwidget.cpp
)
target_include_directories(test_env_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../shared
)
target_link_libraries(test_env_gui PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)

add_test(NAME test_env_gui COMMAND test_env_gui)
