# create config file

include(CMakePackageConfigHelpers)

set(cmakeModulesDir cmake)

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/config.cmake.in
    ${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    PATH_VARS cmakeModulesDir
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
    ${PROJECT_NAME}Config-version.cmake
    COMPATIBILITY SameMinorVersion
)

# install libraries

set_target_properties(
    srscpp
    PROPERTIES
        OUTPUT_NAME "${PROJECT_NAME}_core"
        EXPORT_NAME ${PROJECT_NAME}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION ${PROJECT_VERSION}
)

set_target_properties(
    srs_data
    PROPERTIES
        OUTPUT_NAME "${PROJECT_NAME}_data"
        EXPORT_NAME data
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION ${PROJECT_VERSION}
)

install(TARGETS srs_data EXPORT srsTargets FILE_SET HEADERS)

install(TARGETS srscpp EXPORT srsTargets FILE_SET publicHeaders)

install(
    EXPORT srsTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    NAMESPACE srs::
    FILE srsConfig-targets.cmake
)

install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# install executable

install(TARGETS srs_control srs_check srs_fec_emulator)

# install miscellaneous

find_package(Python3 REQUIRED COMPONENTS Interpreter)

execute_process(
    COMMAND
        "${Python3_EXECUTABLE}" -c
        "import sysconfig; print(sysconfig.get_path('purelib', vars={'base': '', 'platbase': ''}).lstrip('/'))"
    OUTPUT_VARIABLE Python3_INSTALL_SITELIB
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

install(
    FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/message_pb2.py
    DESTINATION "${Python3_INSTALL_SITELIB}/${PROJECT_NAME}"
)
