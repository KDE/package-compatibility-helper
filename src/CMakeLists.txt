# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>

# Target: static library
qt_add_qml_module(packagecompatibilityhelper_static
    STATIC
    URI org.kde.packagecompatibilityhelper
    VERSION 1.0
    QML_FILES
        contents/ui/Main.qml
        contents/ui/InstallPage.qml
)

target_sources(packagecompatibilityhelper_static PUBLIC
    ICompatibilityHelper.cpp
    CompatibilityHelperFactory.cpp
    WindowsCompatibilityHelper.cpp
    BatchCompatibilityHelper.cpp
    RpmCompatibilityHelper.cpp
    DebCompatibilityHelper.cpp
    GenericCompatibilityHelper.cpp
    PackageUtils.cpp
    CompatibilityToolInstaller.cpp
)

target_link_libraries(packagecompatibilityhelper_static PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Svg
    Qt6::Widgets
    Qt6::Xml
    KF6::I18n
    KF6::CoreAddons
    KF6::KIOCore
    KF6::KIOWidgets
    PkgConfig::Flatpak
)
target_include_directories(packagecompatibilityhelper_static PUBLIC ${CMAKE_BINARY_DIR})

# Download app_db.json from ZorinOS
message(STATUS "Fetching latest app_db.json from ZorinOS GitHub...")
file(DOWNLOAD
    "https://raw.githubusercontent.com/ZorinOS/zorin-exec-guard-app-db/refs/heads/master/app_db.json"
    "${CMAKE_CURRENT_BINARY_DIR}/app_db.json"
    STATUS download_status
)
list(GET download_status 0 status_code)
if(NOT status_code EQUAL 0)
    message(FATAL_ERROR "Failed to download app_db.json: ${download_status}")
endif()

# Target: main executable
add_executable(package-compatibility-helper main.cpp)
target_link_libraries(package-compatibility-helper PUBLIC packagecompatibilityhelper_static packagecompatibilityhelper_staticplugin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/app_db.json DESTINATION ${KDE_INSTALL_DATADIR}/${PROJECT_NAME})
install(FILES ${CMAKE_SOURCE_DIR}/data/apps/wine.conf
              ${CMAKE_SOURCE_DIR}/data/apps/dosbox.conf
        DESTINATION ${KDE_INSTALL_DATADIR}/${PROJECT_NAME}/apps)
install(TARGETS package-compatibility-helper ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES org.kde.package-compatibility-helper.desktop DESTINATION ${KDE_INSTALL_APPDIR})
