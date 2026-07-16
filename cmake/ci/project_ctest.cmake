list(APPEND CMAKE_MODULE_PATH ${CTEST_SCRIPT_DIRECTORY})

include(project_ctest_common)

if(NOT DEFINED CONFIGURE_PRESET)
    set(CONFIGURE_PRESET default)
endif()

set(CTEST_CUSTOM_WARNING_EXCEPTION
    ".*Warning in cling::IncrementalParser::CheckABICompatibility().*"
)

ctest_submit(PARTS Start)

set(CONFIGURE_OPTIONS
    "--preset ${CONFIGURE_PRESET}"
    "-DENABLE_COVERAGE=${ENABLE_COVERAGE}"
    ${ADDITIONAL_CONFIGURE_OPTIONS}
)

ctest_configure(
    OPTIONS "${CONFIGURE_OPTIONS}"
    RETURN_VALUE _ctest_config_ret_val
)

ctest_submit(PARTS Configure)

if(_ctest_config_ret_val)
    message(FATAL_ERROR "Configuration failed!")
endif()

ctest_build(RETURN_VALUE _ctest_build_ret_val)

ctest_submit(PARTS Build)

if(_ctest_build_ret_val)
    message(FATAL_ERROR "Compilation failed!")
endif()

ctest_test(RETURN_VALUE _ctest_test_ret_val)

ctest_submit(PARTS Test)

if(_ctest_test_ret_val)
    message(FATAL_ERROR "Some tests failed!")
else()
    message(STATUS "All tests passed!")
endif()
