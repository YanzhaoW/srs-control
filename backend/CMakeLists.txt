# some operating systems don't have static libstdc++.a library
if(BUILD_STATIC)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++ -static-libgcc")
        set(CMAKE_SHARED_LINKER_FLAGS "-static-libstdc++ -static-libgcc")
    else()
        set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++")
        set(CMAKE_SHARED_LINKER_FLAGS "-static-libstdc++")
    endif()
endif()

add_subdirectory(srs)

#-------------------srs-control---------------
add_executable(srs_control srs_control.cpp)
target_link_libraries(
    srs_control
    PRIVATE ${THIRD_PARTY_LIBRARIES} srs::data srs::main
)
target_compile_definitions(
    srs_control
    PUBLIC SRS_PROJECT_VERSION="v${CMAKE_PROJECT_VERSION}"
)
set_target_properties(srs_control PROPERTIES OUTPUT_NAME "srs-control")

#-------------------srs-check---------------
add_executable(srs_check srs_check.cpp)
target_link_libraries(
    srs_check
    PRIVATE ${THIRD_PARTY_LIBRARIES} srs::data srs::main ctre::ctre
)
target_compile_features(srs_check PRIVATE cxx_std_23)
set_target_properties(srs_check PROPERTIES OUTPUT_NAME "srs-check")

#-------------------srs-fec-emulator---------------
add_executable(srs_fec_emulator srs_fec_emulator.cpp)
target_link_libraries(
    srs_fec_emulator
    PRIVATE
        ${THIRD_PARTY_LIBRARIES}
        srs::data
        srs::main
        ${PRIVATE_THIRD_PARTY_LIBRARIES}
)
target_compile_features(srs_fec_emulator PRIVATE cxx_std_23)
set_target_properties(
    srs_fec_emulator
    PROPERTIES OUTPUT_NAME "srs-fec-emulator"
)

#-------------------installation---------------
include(${CMAKE_SOURCE_DIR}/cmake/install_targets.cmake)
