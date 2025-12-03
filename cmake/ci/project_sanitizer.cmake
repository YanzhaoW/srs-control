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
    set(CTEST_BUILD_NAME "${CMAKE_HOST_SYSTEM_NAME} local machine from ${output_user_name}")
endif()

set(CTEST_MEMORYCHECK_SANITIZER_OPTIONS "verbosity=1:symbolize=1:abort_on_error=1:detect_leaks=1")
set(TEST_MODEL "Experimental")

ctest_empty_binary_directory(${CTEST_BINARY_DIRECTORY})

ctest_start(${TEST_MODEL})

set(CONFIGURE_OPTIONS "--preset clang" "-DENABLE_ASAN=${ENABLE_ASAN}"
                      "-DENABLE_TSAN=${ENABLE_TSAN}")

ctest_configure(OPTIONS "${CONFIGURE_OPTIONS}")

ctest_build()

ctest_memcheck()

ctest_submit(PARTS MemCheck)
