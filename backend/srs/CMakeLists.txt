add_library(srscpp SHARED)
target_sources(srscpp PRIVATE Application.cpp)

target_sources(
    srscpp
    PUBLIC FILE_SET
           publicHeaders
           TYPE
           HEADERS
           BASE_DIRS
           ${CMAKE_SOURCE_DIR}/backend)

target_sources(
    srscpp
    PRIVATE FILE_SET
            privateHeaders
            TYPE
            HEADERS
            BASE_DIRS
            ${CMAKE_SOURCE_DIR}/backend)

if(TBB_VERSION VERSION_GREATER "2020.3.1")
    message(STATUS "using oneAPI tbb")
    target_compile_definitions(srscpp PRIVATE USE_ONEAPI_TBB=1)
else()
    message(STATUS "using tbb without oneAPI")
endif()

target_link_libraries(
    srscpp
    PRIVATE $<BUILD_LOCAL_INTERFACE:magic_enum::magic_enum>
            $<BUILD_LOCAL_INTERFACE:gsl::gsl-lite>
            $<BUILD_LOCAL_INTERFACE:Boost::thread>
            $<BUILD_LOCAL_INTERFACE:fmt::fmt>
            $<BUILD_LOCAL_INTERFACE:Taskflow::Taskflow>
            $<BUILD_LOCAL_INTERFACE:spdlog::spdlog>
            $<BUILD_LOCAL_INTERFACE:zpp_bits::zpp_bits>)

add_subdirectory(utils)
add_subdirectory(data)
add_subdirectory(converters)
add_subdirectory(writers)
add_subdirectory(readers)
add_subdirectory(connections)
add_subdirectory(devices)
add_subdirectory(workflow)

target_sources(
    srscpp
    PUBLIC FILE_SET
           publicHeaders
           FILES
           Application.hpp
           format.hpp
           srs.hpp)

target_link_libraries(srscpp PRIVATE $<BUILD_LOCAL_INTERFACE:srs_data>)
target_compile_definitions(srscpp PUBLIC BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
                                         BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY)
target_compile_options(
    srscpp
    PRIVATE -Werror
            -Wall
            -Wextra
            -Wconversion
            -pedantic)

# Set coverage
if(ENABLE_COVERAGE)
    target_compile_options(srscpp PRIVATE --coverage)
    target_link_options(srscpp PRIVATE --coverage)
endif()
