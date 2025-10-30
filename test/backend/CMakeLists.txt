enable_testing()

configure_file(${CMAKE_SOURCE_DIR}/test/data/test_data.bin ${CMAKE_CURRENT_BINARY_DIR} COPYONLY
               USE_SOURCE_PERMISSIONS)

add_library(test_utility SHARED)
target_sources(test_utility PUBLIC FILE_SET HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(unit_test_srs_backend)
target_sources(unit_test_srs_backend PRIVATE test_main.cpp)

add_subdirectory(srs)

add_executable(ctest_output test_output.cpp)
target_link_libraries(ctest_output PRIVATE test_utility CLI11::CLI11)

# CTest

add_test(NAME IntegrationTestEmptyOutput COMMAND ctest_output -l trace)
set_property(TEST IntegrationTestEmptyOutput PROPERTY PASS_REGULAR_EXPRESSION
                                                      "Application has exited.")
set_property(
    TEST IntegrationTestEmptyOutput
    PROPERTY FAIL_REGULAR_EXPRESSION "Process: Starting Raw2DelimRaw converter."
             "Process: Shutting down Raw2DelimRaw converter.")

add_test(NAME IntegrationTestBinOutput COMMAND ctest_output -l trace -o test_output.bin)
set_property(
    TEST IntegrationTestBinOutput
    PROPERTY
        PASS_REGULAR_EXPRESSION
        "Process: Starting Raw2DelimRaw converter.*Process: Shutting down Raw2DelimRaw converter.*Application has exited."
)
