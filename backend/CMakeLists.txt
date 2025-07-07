# some operating systems don't have static libstdc++.a library
if(BUILD_STATIC)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libstdc++")
    set(CMAKE_SHARED_LINKER_FLAGS "-static-libstdc++")
endif()

add_subdirectory(srs)

add_executable(srs_control main.cpp)
target_link_libraries(
    srs_control
    PRIVATE srscpp
            spdlog::spdlog
            CLI11::CLI11
            Boost::thread
            glaze::glaze
            magic_enum::magic_enum
            srs_data)
target_compile_definitions(srs_control PUBLIC SRS_PROJECT_VERSION="v${CMAKE_PROJECT_VERSION}")

include(${CMAKE_SOURCE_DIR}/cmake/install_targets.cmake)
