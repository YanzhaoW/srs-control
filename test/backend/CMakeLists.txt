enable_testing()
include(GoogleTest)

configure_file(${CMAKE_SOURCE_DIR}/test/data/test_data.bin ${CMAKE_CURRENT_BINARY_DIR} COPYONLY
               USE_SOURCE_PERMISSIONS)

add_library(test_utility SHARED)
target_sources(test_utility PUBLIC FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(unit_test_srs_backend)
target_sources(unit_test_srs_backend PRIVATE test_main.cpp)

add_subdirectory(srs)

add_executable(ctest_output test_output.cpp)
target_link_libraries(ctest_output PRIVATE test_utility CLI11::CLI11)

# Add GTest to ctest

gtest_discover_tests(unit_test_srs_backend)

# CTest

add_test(NAME IntegrationTestEmptyOutput COMMAND ctest_output -l trace)
set_tests_properties(
    IntegrationTestEmptyOutput
    PROPERTIES
        FAIL_REGULAR_EXPRESSION
        "Process: Starting Raw2DelimRaw converter;Process: Shutting down Raw2DelimRaw converter."
        PROPERTY_REGULAR_EXPRESSION
        "Application has exited."
        TIMEOUT
        8)

add_test(NAME IntegrationTestBinOutput COMMAND ctest_output -l trace -o test_output.bin)
set_tests_properties(
    IntegrationTestBinOutput
    PROPERTIES
        PROPERTY_REGULAR_EXPRESSION
        "Process: Starting Raw2DelimRaw converter.*Process: Shutting down Raw2DelimRaw converter.*Application has exited."
        TIMEOUT
        8)

add_test(NAME IntegrationTestJSONOutput COMMAND ctest_output -l trace -o test_output.json)
set_tests_properties(IntegrationTestJSONOutput PROPERTIES TIMEOUT 8)

add_test(NAME IntegrationTestProtoDelimOutput COMMAND ctest_output -l trace -o test_output.binpb)
set_tests_properties(IntegrationTestProtoDelimOutput PROPERTIES TIMEOUT 8)

add_test(NAME IntegrationTestProtoUDPOutput COMMAND ctest_output -l trace -o 127.0.0.1:12345)
set_tests_properties(IntegrationTestProtoUDPOutput PROPERTIES TIMEOUT 8)

add_test(NAME IntegrationTestRawDataUDPOutput COMMAND ctest_output -l trace -o 127.0.0.1:?12344)
set_tests_properties(IntegrationTestRawDataUDPOutput PROPERTIES TIMEOUT 8)

if(ROOT_FOUND)
    add_test(NAME IntegrationTestRootOutput COMMAND ctest_output -l trace -o test_output.root)
    set_tests_properties(IntegrationTestRootOutput PROPERTIES TIMEOUT 8)
endif()
