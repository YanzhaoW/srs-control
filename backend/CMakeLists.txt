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

add_executable(srs_control main.cpp)
target_link_libraries(
    srs_control
    PRIVATE
        CLI11::CLI11
        asio::asio
        glaze::glaze
        magic_enum::magic_enum
        spdlog::spdlog
        srs_data
        srscpp
)
target_compile_definitions(
    srs_control
    PUBLIC SRS_PROJECT_VERSION="v${CMAKE_PROJECT_VERSION}"
)

add_executable(srs_check check_log.cpp)

target_link_libraries(
    srs_check
    PRIVATE
        CLI11::CLI11
        asio::asio
        magic_enum::magic_enum
        re2::re2
        spdlog::spdlog
        srscpp
)

include(${CMAKE_SOURCE_DIR}/cmake/install_targets.cmake)
