# Re-use CDash server details we already have
include(${CTEST_SCRIPT_DIRECTORY}/../CTestConfig.cmake)

# Basic information every run should set, values here are just examples
if(DEFINED SITE_NAME)
    set(CTEST_SITE ${SITE_NAME})
else()
    site_name(CTEST_SITE)
endif()

if(NOT DEFINED CTEST_BUILD_NAME)
    set(CTEST_BUILD_NAME "${CMAKE_HOST_SYSTEM_NAME} local machine")
endif()
if(NOT DEFINED CTEST_CONFIGURATION_TYPE)
    set(CTEST_CONFIGURATION_TYPE Debug)
endif()
set(CTEST_SOURCE_DIRECTORY "${CTEST_SCRIPT_DIRECTORY}/..")
set(CTEST_BINARY_DIRECTORY "${CTEST_SCRIPT_DIRECTORY}/../build")
set(CTEST_CMAKE_GENERATOR Ninja)
set(CTEST_COVERAGE_COMMAND "gcov")

set(CONFIGURE_OPTIONS
    "-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=${CTEST_SCRIPT_DIRECTORY}/../utils/conan/conan_provider.cmake"
    "-DCONAN_INSTALL_ARGS=--build=missing\;-scompiler.cppstd=gnu23"
    "-DENABLE_COVERAGE=ON"
    ${ADDITIONAL_CONFIGURE_OPTIONS})

ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})
ctest_start(Experimental)
ctest_configure(OPTIONS "${CONFIGURE_OPTIONS}")
ctest_build()
ctest_test()
ctest_submit()
