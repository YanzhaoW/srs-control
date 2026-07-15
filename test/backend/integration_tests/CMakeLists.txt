configure_file(
    ${CMAKE_SOURCE_DIR}/test/data/test_data.bin
    ${CMAKE_CURRENT_BINARY_DIR}
    COPYONLY
    USE_SOURCE_PERMISSIONS
)

function(test_command ret_val)
    include(CMakeParseArguments)
    cmake_parse_arguments(
        ARG
        ""
        "EMULATOR_CONFIG;CONTROL_CONFIG"
        "CONTROL_ARGS"
        "${ARGN}"
    )
    string(JOIN " " control_arg_str ${ARG_CONTROL_ARGS})
    set(${ret_val}
        "$<TARGET_FILE:srs_fec_emulator> ${CMAKE_CURRENT_SOURCE_DIR}/${ARG_EMULATOR_CONFIG} > /dev/null & $<TARGET_FILE:srs_control> -c ${CMAKE_CURRENT_SOURCE_DIR}/${ARG_CONTROL_CONFIG} ${control_arg_str}"
        PARENT_SCOPE
    )
endfunction()

# cmake-format: off
test_command(
    command_str
    EMULATOR_CONFIG
    "test_single_fec_emulator.yaml"
    CONTROL_CONFIG
    "test_single_fec_control.yaml"
    CONTROL_ARGS
    -l trace -r 3
)
# cmake-format: on

add_test(NAME IntegrationTestEmptyOutput COMMAND bash -c "${command_str}")

set_tests_properties(
    IntegrationTestEmptyOutput
    PROPERTIES
        FAIL_REGULAR_EXPRESSION
            "Process: Starting Raw2DelimRaw converter;Process: Shutting down Raw2DelimRaw converter."
        PROPERTY_REGULAR_EXPRESSION "Application has exited."
        TIMEOUT 20
)

# cmake-format: off
test_command(
    command_str
    EMULATOR_CONFIG
    "test_single_fec_emulator.yaml"
    CONTROL_CONFIG
    "test_single_fec_control.yaml"
    CONTROL_ARGS
    -l trace -r 3 -o test_output.bin
)
# cmake-format: on
add_test(NAME IntegrationTestBinOutput COMMAND bash -c "${command_str}")
set_tests_properties(
    IntegrationTestBinOutput
    PROPERTIES
        PROPERTY_REGULAR_EXPRESSION
            "Process: Starting Raw2DelimRaw converter.*Process: Shutting down Raw2DelimRaw converter.*Application has exited."
        TIMEOUT 20
)

# cmake-format: off
test_command(
    command_str
    EMULATOR_CONFIG
    "test_single_fec_emulator.yaml"
    CONTROL_CONFIG
    "test_single_fec_control.yaml"
    CONTROL_ARGS
    -l trace -r 3 -o test_output.json
)
# cmake-format: on
add_test(NAME IntegrationTestJSONOutput COMMAND bash -c "${command_str}")
set_tests_properties(IntegrationTestJSONOutput PROPERTIES TIMEOUT 20)

# cmake-format: off
test_command(
    command_str
    EMULATOR_CONFIG
    "test_single_fec_emulator.yaml"
    CONTROL_CONFIG
    "test_single_fec_control.yaml"
    CONTROL_ARGS
    -l trace -r 3 -o test_output.binpb
)
# cmake-format: on
add_test(NAME IntegrationTestProtoDelimOutput COMMAND bash -c "${command_str}")
set_tests_properties(IntegrationTestProtoDelimOutput PROPERTIES TIMEOUT 20)

# cmake-format: off
test_command(
    command_str
    EMULATOR_CONFIG
    "test_single_fec_emulator.yaml"
    CONTROL_CONFIG
    "test_single_fec_control.yaml"
    CONTROL_ARGS
    -l trace -r 3 -o 127.0.0.1:12345
)
# cmake-format: on
add_test(NAME IntegrationTestProtoUDPOutput COMMAND bash -c "${command_str}")
set_tests_properties(IntegrationTestProtoUDPOutput PROPERTIES TIMEOUT 20)

# cmake-format: off
test_command(
    command_str
    EMULATOR_CONFIG
    "test_single_fec_emulator.yaml"
    CONTROL_CONFIG
    "test_single_fec_control.yaml"
    CONTROL_ARGS
    -l trace -r 3 -o 127.0.0.1:?11231
)
# cmake-format: on
add_test(NAME IntegrationTestRawDataUDPOutput COMMAND bash -c "${command_str}")
set_tests_properties(IntegrationTestRawDataUDPOutput PROPERTIES TIMEOUT 20)

if(ROOT_FOUND)
    # cmake-format: off
    test_command(
        command_str
        EMULATOR_CONFIG
        "test_single_fec_emulator.yaml"
        CONTROL_CONFIG
        "test_single_fec_control.yaml"
        CONTROL_ARGS
        -l trace -r 3 -o test_output.root
    )
    # cmake-format: on
    add_test(NAME IntegrationTestRootOutput COMMAND bash -c "${command_str}")
    set_tests_properties(IntegrationTestRootOutput PROPERTIES TIMEOUT 20)
endif()

if(NOT APPLE)
    # cmake-format: off
    test_command(
        command_str
        EMULATOR_CONFIG
        "test_multi_data_ports_emulator.yaml"
        CONTROL_CONFIG
        "test_multi_data_ports_control.yaml"
        CONTROL_ARGS
        -l trace -r 3 -o test_multi_output.bin
    )
    # cmake-format: on
    add_test(
        NAME IntegrationTestBinOutputMultiDataPorts
        COMMAND bash -c "${command_str}"
    )
    set_tests_properties(
        IntegrationTestBinOutputMultiDataPorts
        PROPERTIES TIMEOUT 20
    )
endif()
