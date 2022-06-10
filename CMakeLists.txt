# SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
#
# SPDX-License-Identifier: BSD-2-Clause

cmake_minimum_required(VERSION 3.16)

project(flatpakkcm)

set(QT_MIN_VERSION "5.15.0")
set(KF5_MIN_VERSION "5.88.0")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
set(KF_IGNORE_PLATFORM_CHECK ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Flatpak REQUIRED IMPORTED_TARGET flatpak>=0.11.8)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Quick
    Svg
)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    I18n
    KCMUtils
    Declarative
    Config
)

set(flatpakkcm_SRCS
    kcm.cpp
    flatpakreference.cpp
)

add_library(kcm_flatpak MODULE ${flatpakkcm_SRCS})

target_link_libraries(kcm_flatpak
    Qt5::Core
    KF5::CoreAddons
    KF5::I18n
    KF5::QuickAddons
    PkgConfig::Flatpak
)

install(TARGETS kcm_flatpak DESTINATION ${KDE_INSTALL_PLUGINDIR}/plasma/kcms/systemsettings)

kpackage_install_package(package kcm_flatpak kcms)
