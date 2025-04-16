# create config file

include(CMakePackageConfigHelpers)

set(cmakeModulesDir cmake)

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/srsConfig.cmake.in srsConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/srs
    PATH_VARS cmakeModulesDir
    NO_SET_AND_CHECK_MACRO NO_CHECK_REQUIRED_COMPONENTS_MACRO)

write_basic_package_version_file(srsConfig-version.cmake COMPATIBILITY SameMinorVersion)

# install libraries

set_target_properties(
    srscpp
    PROPERTIES OUTPUT_NAME srs
               EXPORT_NAME srs
               SOVERSION ${PROJECT_VERSION_MAJOR}
               VERSION ${PROJECT_VERSION})

install(
    TARGETS srs_data
    EXPORT srsTargets
    FILE_SET HEADERS)

install(
    TARGETS srscpp
    EXPORT srsTargets
    FILE_SET publicHeaders)

install(
    EXPORT srsTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/srs
    NAMESPACE srs::
    FILE srsConfig-targets.cmake)

# install(IMPORTED_RUNTIME_ARTIFACTS TBB::tbb LIBRARY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/srsConfig.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/srsConfig-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/srs)

# install executable

install(TARGETS srs_control)

install(PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/srs_check_udp
                 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/srs_check_binpb
        DESTINATION ${CMAKE_INSTALL_BINDIR})

# install miscellaneous

install(FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/message_pb2.py DESTINATION ${CMAKE_INSTALL_BINDIR})
