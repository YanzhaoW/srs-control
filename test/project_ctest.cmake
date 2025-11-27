# Re-use CDash server details we already have
include(${CTEST_SCRIPT_DIRECTORY}/../CTestConfig.cmake)

# Basic information every run should set, values here are just examples
if(DEFINED SITE_NAME)
    set(CTEST_SITE ${SITE_NAME})
else()
    site_name(CTEST_SITE)
endif()

if(NOT DEFINED CTEST_BUILD_NAME)
    execute_process(COMMAND git config --global user.name OUTPUT_VARIABLE output_user_name)
    string(STRIP ${output_user_name} output_user_name)
    set(CTEST_BUILD_NAME "${CMAKE_HOST_SYSTEM_NAME} local machine ${output_user_name}")
endif()

if(NOT DEFINED CTEST_CONFIGURATION_TYPE)
    set(CTEST_CONFIGURATION_TYPE Debug)
endif()

if(NOT DEFINED CONFIGURE_PRESET)
    set(CONFIGURE_PRESET default)
endif()

if(NOT DEFINED TEST_MODEL)
    set(TEST_MODEL "Experimental")
endif()

set(CTEST_SOURCE_DIRECTORY "${CTEST_SCRIPT_DIRECTORY}/..")
set(CTEST_BINARY_DIRECTORY "${CTEST_SCRIPT_DIRECTORY}/../build")
set(CTEST_CMAKE_GENERATOR Ninja)
set(CTEST_COVERAGE_COMMAND "gcov")

set(CONFIGURE_OPTIONS "--preset ${CONFIGURE_PRESET}" "-DENABLE_COVERAGE=ON"
                      ${ADDITIONAL_CONFIGURE_OPTIONS})

ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})

ctest_start(${TEST_MODEL})

ctest_configure(OPTIONS "${CONFIGURE_OPTIONS}")

ctest_submit(PARTS Start Configure)

ctest_build()

ctest_submit(PARTS Build)

ctest_test(RETURN_VALUE _ctest_test_ret_val)

ctest_submit(RETRY_COUNT 3 RETRY_DELAY 2)

if(_ctest_test_ret_val)
    message(FATAL_ERROR "Some tests failed!")
endif()
