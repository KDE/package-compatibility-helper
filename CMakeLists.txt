# SPDX-FileCopyrightText: 2025 Thomas Duckworth <tduck@filotimoproject.org>
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(package-compatibility-helper VERSION 0.10.0)

include(FeatureSummary)

set(QT6_MIN_VERSION 6.5.0)
set(KF6_MIN_VERSION 6.12.0)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH}
                      ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMSetupVersion)
include(ECMFindQmlModule)
include(KDEGitCommitHooks)
include(KDEClangFormat)

ecm_setup_version(${PROJECT_VERSION}
  VARIABLE_PREFIX PACKAGE_COMPATIBILITY_HELPER
  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/src/version-package-compatibility-helper.h"
)

set(QT_EXTRA_COMPONENTS)
list(APPEND QT_EXTRA_COMPONENTS Widgets)

find_package(
  Qt6 ${QT6_MIN_VERSION} REQUIRED
  COMPONENTS Core
             Gui
             Network
             Qml
             QuickControls2
             Svg
             Xml
             ${QT_EXTRA_COMPONENTS})
find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS Kirigami CoreAddons
                                                        I18n KIO)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Flatpak REQUIRED IMPORTED_TARGET flatpak>=1.4)

qt_policy(SET QTP0001 NEW)

ecm_find_qmlmodule(org.kde.kirigamiaddons.formcard 1.0)

add_subdirectory(src)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/directories.h.in
               ${CMAKE_CURRENT_SOURCE_DIR}/src/directories.h @ONLY)

ki18n_install(po)

# Launcher used by the shims in trees/. It chains to
# `package-compatibility-helper --install <name>` when the Flatpak is missing.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/data/package-compatibility-helper-run.in
               ${CMAKE_CURRENT_BINARY_DIR}/data/package-compatibility-helper-run @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/data/package-compatibility-helper-run
        DESTINATION ${KDE_INSTALL_LIBEXECDIR})

# Integration trees (command shims, MIME handlers, binfmt rules plus an
# installer config). The install logic lives in cmake/PackageCompatibilityHelperInstallTrees.cmake.
set(PACKAGE_COMPATIBILITY_HELPER_TREES "all" CACHE STRING
    "Semicolon-separated list of trees to install, 'all', or empty for none")

if(NOT PACKAGE_COMPATIBILITY_HELPER_TREES STREQUAL "")
    include(PackageCompatibilityHelperInstallTrees)
    package_compatibility_helper_install_trees(TREES ${PACKAGE_COMPATIBILITY_HELPER_TREES})
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES src/*.cpp src/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
