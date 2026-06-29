# some operating systems don't have static libstdc++.a library
if(BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++")
    set(CMAKE_SHARED_LINKER_FLAGS "-static-libstdc++")
endif()

add_subdirectory(srs)

add_executable(srs_control main.cpp)
target_link_libraries(
    srs_control
    PRIVATE
        Boost::thread
        CLI11::CLI11
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

add_executable(srs_check_log check_log.cpp)

target_link_libraries(
    srs_check_log
    PRIVATE
        CLI11::CLI11
        magic_enum::magic_enum
        spdlog::spdlog
        Boost::headers
        srscpp
        re2::re2
)

include(${CMAKE_SOURCE_DIR}/cmake/install_targets.cmake)
