enable_testing()

add_library(test_utility SHARED)
target_sources(
    test_utility
    PUBLIC FILE_SET HEADERS BASE_DIRS ${CMAKE_SOURCE_DIR}
)

add_subdirectory(utils)
add_subdirectory(unit_tests)
add_subdirectory(integration_tests)
