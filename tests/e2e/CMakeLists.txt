find_package(Python3 REQUIRED)

add_test(NAME e2e_wm_gui COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/e2e_test_runner.py WmGuiTests --wm-gui $<TARGET_FILE:nv-wm-gui>)
add_test(NAME e2e_notifications_gui COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/e2e_test_runner.py NotificationsGuiTests --notifications-gui $<TARGET_FILE:nv-notifications-gui>)
add_test(NAME e2e_launcher_gui COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/e2e_test_runner.py LauncherGuiTests --launcher-gui $<TARGET_FILE:nv-launcher-gui>)
add_test(NAME e2e_lockscreen_gui COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/e2e_test_runner.py LockscreenGuiTests --lockscreen-gui $<TARGET_FILE:nv-lockscreen-gui>)
add_test(NAME e2e_screenshot_gui COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/e2e_test_runner.py ScreenshotGuiTests --screenshot-gui $<TARGET_FILE:nv-screenshot-gui>)
add_test(NAME e2e_cross_feature COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/e2e_test_runner.py CrossFeatureTests
    --wm-gui $<TARGET_FILE:nv-wm-gui>
    --notifications-gui $<TARGET_FILE:nv-notifications-gui>
    --launcher-gui $<TARGET_FILE:nv-launcher-gui>
    --lockscreen-gui $<TARGET_FILE:nv-lockscreen-gui>
    --screenshot-gui $<TARGET_FILE:nv-screenshot-gui>)
add_test(NAME e2e_real_world COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/e2e_test_runner.py RealWorldTests
    --wm-gui $<TARGET_FILE:nv-wm-gui>
    --notifications-gui $<TARGET_FILE:nv-notifications-gui>
    --launcher-gui $<TARGET_FILE:nv-launcher-gui>
    --lockscreen-gui $<TARGET_FILE:nv-lockscreen-gui>
    --screenshot-gui $<TARGET_FILE:nv-screenshot-gui>)
