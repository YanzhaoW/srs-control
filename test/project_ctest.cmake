# Re-use CDash server details we already have
include(${CTEST_SCRIPT_DIRECTORY}/../CTestConfig.cmake)

# Basic information every run should set, values here are just examples
if(DEFINED SITE_NAME)
    set(CTEST_SITE ${SITE_NAME})
else()
    site_name(CTEST_SITE)
endif()

if(NOT DEFINED ENABLE_COVERAGE)
    set(ENABLE_COVERAGE OFF)
endif()

if(NOT DEFINED CTEST_BUILD_NAME)
    execute_process(COMMAND git config --global user.name OUTPUT_VARIABLE output_user_name)
    string(STRIP ${output_user_name} output_user_name)
    set(CTEST_BUILD_NAME "${CMAKE_HOST_SYSTEM_NAME} local machine from ${output_user_name}")
endif()

set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS "verbosity=1:symbolize=1:abort_on_error=1:detect_leaks=1")

if(NOT DEFINED CTEST_CONFIGURATION_TYPE)
    set(CTEST_CONFIGURATION_TYPE Debug)
endif()

if(NOT DEFINED CONFIGURE_PRESET)
    set(CONFIGURE_PRESET default)
endif()

if(NOT DEFINED TEST_MODEL)
    set(TEST_MODEL "Experimental")
endif()

set(CTEST_CUSTOM_WARNING_EXCEPTION
    ".*Warning in cling::IncrementalParser::CheckABICompatibility().*")

set(CTEST_SOURCE_DIRECTORY "${CTEST_SCRIPT_DIRECTORY}/..")
set(CTEST_BINARY_DIRECTORY "${CTEST_SCRIPT_DIRECTORY}/../build")
set(CTEST_CMAKE_GENERATOR Ninja)
set(CTEST_COVERAGE_COMMAND "gcov")

ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})

ctest_start(${TEST_MODEL})

ctest_submit(PARTS Start)

set(CONFIGURE_OPTIONS "--preset ${CONFIGURE_PRESET}" "-DENABLE_COVERAGE=${ENABLE_COVERAGE}"
                      ${ADDITIONAL_CONFIGURE_OPTIONS})

ctest_configure(OPTIONS "${CONFIGURE_OPTIONS}")

ctest_submit(PARTS Configure)

ctest_build()

ctest_submit(PARTS Build)

ctest_test(RETURN_VALUE _ctest_test_ret_val)

if(ENABLE_COVERAGE)
    ctest_coverage(QUIET)
endif()

if(_ctest_test_ret_val)
    message(FATAL_ERROR "Some tests failed!")
endif()

ctest_submit(PARTS Coverage)

# sanitizers

if(NOT ENABLE_COVERAGE)
    set(CONFIGURE_OPTIONS "-DENABLE_ASAN=ON" "-DENABLE_TSAN=OFF")
    ctest_configure(OPTIONS "${CONFIGURE_OPTIONS}")
    ctest_build()

    set(CTEST_MEMORYCHECK_TYPE AddressSanitizer)
    ctest_memcheck()
    ctest_submit(PARTS MemCheck)

    set(CTEST_MEMORYCHECK_TYPE LeakSanitizer)
    ctest_memcheck()
    ctest_submit(PARTS MemCheck)

    set(CTEST_MEMORYCHECK_TYPE UndefinedBehaviorSanitizer)
    ctest_memcheck()
    ctest_submit(PARTS MemCheck)

    set(CONFIGURE_OPTIONS "-DENABLE_ASAN=OFF" "-DENABLE_TSAN=ON")
    ctest_configure(OPTIONS "${CONFIGURE_OPTIONS}")
    ctest_build()
    set(CTEST_MEMORYCHECK_TYPE ThreadSanitizer)
    ctest_memcheck()
    ctest_submit(PARTS MemCheck)
endif()
