cmake_minimum_required(VERSION 3.20)
project(naravisuals-labwc-lxqt
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "LXQt + Labwc Wayland desktop environment GUI suite"
    HOMEPAGE_URL "https://github.com/naranyala/naravisuals-labwc-lxqt"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- App subdirectories ----
add_subdirectory(apps/audio-gui)
add_subdirectory(apps/autorun-gui)
add_subdirectory(apps/bluetooth-gui)
add_subdirectory(apps/control-center)
add_subdirectory(apps/default-apps-gui)
add_subdirectory(apps/desktop-gui)
add_subdirectory(apps/disk-gui)
add_subdirectory(apps/display-gui)
add_subdirectory(apps/env-gui)
add_subdirectory(apps/font-gui)
add_subdirectory(apps/input-gui)
add_subdirectory(apps/keybinds-gui)
add_subdirectory(apps/launcher-gui)
add_subdirectory(apps/lockscreen-gui)
add_subdirectory(apps/log-gui)
add_subdirectory(apps/mime-gui)
add_subdirectory(apps/network-gui)
add_subdirectory(apps/notifications-gui)
add_subdirectory(apps/ntfs-gui)
add_subdirectory(apps/power-gui)
add_subdirectory(apps/screenshot-gui)
add_subdirectory(apps/sddm-gui)
add_subdirectory(apps/service-gui)
add_subdirectory(apps/sync-gui)
add_subdirectory(apps/theme-gui)
add_subdirectory(apps/theme-manager)
add_subdirectory(apps/wallpaper-gui)
add_subdirectory(apps/wm-gui)

enable_testing()
add_subdirectory(tests/e2e)

# ---- CPack packaging ----
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "LXQt + Labwc Wayland desktop environment GUI suite")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/naranyala/naravisuals-labwc-lxqt")
set(CPACK_PACKAGE_VENDOR "naranyala")
set(CPACK_PACKAGE_CONTACT "naranyala")
set(CPACK_PACKAGE_LICENSE "GPL-2.0-only")
# set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

# Debian generator
set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT")
set(CPACK_DEBIAN_PACKAGE_SECTION "x11")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "qt6-base-dev (>= 6.4), libqt6widgets6 (>= 6.4), libqt6gui6 (>= 6.4), libqt6core6 (>= 6.4)"
)
set(CPACK_DEBIAN_PACKAGE_SUGGESTS
    "kvantum, qt6ct, labwc, lxqt-panel, dunst, rofi-wayland, swaybg, swayidle, swaylock"
)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_SOURCE_DIR}/debian/postinst"
)

# RPM generator
set(CPACK_RPM_PACKAGE_GROUP "User Interface/Desktops")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-2.0-only")
set(CPACK_RPM_PACKAGE_REQUIRES
    "qt6-qtbase >= 6.4, qt6-qtbase-gui >= 6.4"
)
set(CPACK_RPM_FILE_NAME "RPM-DEFAULT")
set(CPACK_RPM_CHANGELOG_FILE "${CMAKE_SOURCE_DIR}/rpm/changelog")

# Archive generator (for AppImage / tarball)
set(CPACK_ARCHIVE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-src")
set(CPACK_ARCHIVE_COMPONENT_INSTALL OFF)

# Component install for AppImage bundle
set(CPACK_COMPONENTS_ALL apps configs)
install(CODE "message(STATUS \"Packaging naravisuals-labwc-lxqt v${PROJECT_VERSION}\")")

set(CPACK_GENERATOR "DEB;RPM;TXZ")

include(CPack)
