add_library(srscpp SHARED)
target_sources(srscpp PRIVATE Application.cpp)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(srscpp PRIVATE "-fclang-abi-compat=17")
endif()

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

target_link_libraries(
    srscpp
    PRIVATE $<BUILD_LOCAL_INTERFACE:magic_enum::magic_enum>
            $<BUILD_LOCAL_INTERFACE:gsl::gsl-lite>
            $<BUILD_LOCAL_INTERFACE:Boost::thread>
            $<BUILD_LOCAL_INTERFACE:fmt::fmt>
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
