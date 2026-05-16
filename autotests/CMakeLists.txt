# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

if(NOT BUILD_TESTING)
    return()
endif()

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Test)
include(ECMAddTests)
remove_definitions(-DQT_NO_CAST_FROM_ASCII)

add_test(NAME generate_overrides
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/overrides.in ${CMAKE_CURRENT_BINARY_DIR}/fixtures/overrides
)

ecm_add_test(flatpakpermissiontest.cpp
    TEST_NAME flatpakpermissiontest
    LINK_LIBRARIES Qt6::Test kcm_flatpak_static
)
set_tests_properties(flatpakpermissiontest PROPERTIES
    ENVIRONMENT "LANGUAGE=fr"
    DEPENDS generate_overrides
    REQUIRED_FILES ${CMAKE_CURRENT_BINARY_DIR}/fixtures/overrides/org.gnome.dfeet
)

ecm_add_test(FlatpakSimpleEntryTest.cpp
    TEST_NAME FlatpakSimpleEntryTest
    LINK_LIBRARIES Qt6::Test kcm_flatpak_static)

ecm_add_test(flatpakfilesystemstest.cpp
    TEST_NAME flatpakfilesystemstest
    LINK_LIBRARIES Qt6::Test kcm_flatpak_static)

# kcmshell6 < 6.27.0 doesn't know the --smoke-test param, so skip this test for
# now.
# TODO: Drop this when KF6_MIN_VERSION >= 6.27.0
if(KF6KCMUtils_VERSION VERSION_GREATER_EQUAL "6.27.0")
    add_test(NAME kcm_smoketest COMMAND kcmshell6 --smoke-test kcm_app-permissions)
    set_tests_properties(kcm_smoketest PROPERTIES
        ENVIRONMENT_MODIFICATION QT_PLUGIN_PATH=path_list_prepend:${CMAKE_BINARY_DIR}/bin
    )
endif()
