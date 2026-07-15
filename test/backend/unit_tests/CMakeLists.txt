include(Catch)

add_executable(unit_test_srs_backend)

target_sources(
    unit_test_srs_backend
    PUBLIC FILE_SET HEADERS BASE_DIRS ${CMAKE_SOURCE_DIR}/backend
    PRIVATE UnitTestWriter.cpp UnitTestStruct.cpp UnitTestConfig.cpp
)
target_link_libraries(
    unit_test_srs_backend
    PRIVATE
        Catch2::Catch2WithMain
        srs::main
        ${THIRD_PARTY_LIBRARIES}
        ${PRIVATE_THIRD_PARTY_LIBRARIES}
)
target_compile_definitions(
    unit_test_srs_backend
    PRIVATE
        TEST_JSON_CONFIG_INPUT_FILENAME="${CMAKE_CURRENT_SOURCE_DIR}/test_config_input.json"
        TEST_YAML_CONFIG_INPUT_FILENAME="${CMAKE_CURRENT_SOURCE_DIR}/test_config_input.yaml"
)

catch_discover_tests(unit_test_srs_backend)
