# SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
# SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
#
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

project(flatpakkcm)

set(QT_MIN_VERSION "5.15.2")
set(KF5_MIN_VERSION "5.102.0")

add_definitions(-DTRANSLATION_DOMAIN="kcm_flatpak")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
set(KF_IGNORE_PLATFORM_CHECK ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Flatpak REQUIRED IMPORTED_TARGET flatpak>=0.11.8)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(FeatureSummary)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Quick
    Svg
)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    I18n
    KCMUtils
    Declarative
    Config
)

# Can't use regular KDE_INSTALL_FULL_LOCALSTATEDIR because it is wrong in developer setup.
# This variable has to point at a directory which Flapak itself was configured with.
set(LOCALSTATEDIR "/var")
configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)

add_library(kcm_flatpak_static STATIC
    flatpakreference.cpp flatpakreference.h
    flatpakpermission.cpp flatpakpermission.h
    flatpakhelper.cpp flatpakhelper.h
    flatpakcommon.cpp flatpakcommon.h
)
set_property(TARGET kcm_flatpak_static PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(kcm_flatpak_static
    Qt::Core
    KF5::CoreAddons
    KF5::ConfigCore
    KF5::I18n
    KF5::QuickAddons
    PkgConfig::Flatpak
    Qt::DBus
)

add_library(kcm_flatpak MODULE
    kcm.cpp kcm.h
)
target_link_libraries(kcm_flatpak kcm_flatpak_static)

install(TARGETS kcm_flatpak DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/kcms/systemsettings)

kpackage_install_package(package kcm_flatpak kcms)

kcmutils_generate_desktop_file(kcm_flatpak)

ki18n_install(po)

add_subdirectory(autotests)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
