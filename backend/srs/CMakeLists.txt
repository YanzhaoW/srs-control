add_library(srscpp SHARED)
add_library(srs::main ALIAS srscpp)

target_sources(
    srscpp
    PRIVATE Application.cpp
    PUBLIC
        FILE_SET publicHeaders
            TYPE HEADERS
            BASE_DIRS ${CMAKE_SOURCE_DIR}/backend
    PRIVATE
        FILE_SET privateHeaders
            TYPE HEADERS
            BASE_DIRS ${CMAKE_SOURCE_DIR}/backend
)

target_link_libraries(srscpp PRIVATE ${PRIVATE_THIRD_PARTY_LIBRARIES})

add_subdirectory(utils)
add_subdirectory(data)
add_subdirectory(converters)
add_subdirectory(sinks)
add_subdirectory(readers)
add_subdirectory(connections)
add_subdirectory(devices)
add_subdirectory(workflow)
add_subdirectory(emulators)

target_sources(
    srscpp
    PUBLIC FILE_SET publicHeaders FILES Application.hpp format.hpp srs.hpp
)

target_link_libraries(srscpp PRIVATE $<BUILD_LOCAL_INTERFACE:srs_data>)
target_compile_options(
    srscpp
    PRIVATE -Werror -Wall -Wextra -Wconversion -pedantic
)

# Set coverage
if(ENABLE_COVERAGE)
    target_compile_options(srscpp PRIVATE --coverage -fprofile-update=atomic)
    target_link_options(srscpp PRIVATE --coverage -fprofile-update=atomic)
endif()

if(ENABLE_ASAN)
    target_compile_options(
        srscpp
        PUBLIC -fno-omit-frame-pointer -fsanitize=address,undefined,leak
    )
    target_link_options(srscpp PUBLIC -fsanitize=address,undefined,leak)
endif()

if(ENABLE_TSAN)
    target_compile_options(
        srscpp
        PUBLIC -fno-omit-frame-pointer -fsanitize=thread
    )
    target_link_options(srscpp PUBLIC -fsanitize=thread)
endif()
