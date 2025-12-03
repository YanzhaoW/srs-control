option(USE_ROOT "Force to use ROOT dependency." OFF)
option(NO_ROOT "Disable the usage of ROOT dependency." OFF)
option(BUILD_STATIC "Enable static linking of libstdc++." OFF)
option(
    USE_SYSTEM_PROTOBUF
    "Use system provided protobuf instead of the one from the package manager."
    OFF
)
option(
    USE_SYSTEM_BOOST
    "Use system provided boost instead of the one from the package manager."
    OFF
)
option(
    USE_SYSTEM_LIBRARIES
    "Use system provided libraries instead of ones from the package manager."
    OFF
)
option(ENABLE_TEST "Enable testing framework of the project." ON)
option(BUILD_DOC "Build the documentation for this project." OFF)
option(BUILD_ONLY_DOC "Only build the documentation for this project." OFF)
option(ENABLE_COVERAGE "Enable coverage flags" OFF)

set(SPHINX_BUILDER "html" CACHE STRING "Set the builder for sphnix-build.")

set(ENV{CMAKE_ENABLE_TEST} ${ENABLE_TEST})

if(USE_SYSTEM_LIBRARIES)
    set(ENV{CMAKE_USE_SYSTEM_PROTOBUF} ON)
    set(ENV{CMAKE_USE_SYSTEM_BOOST} ON)
else()
    set(ENV{CMAKE_USE_SYSTEM_PROTOBUF} ${USE_SYSTEM_PROTOBUF})
    set(ENV{CMAKE_USE_SYSTEM_BOOST} ${USE_SYSTEM_BOOST})
endif()

message(STATUS "CMAKE_USE_SYSTEM_PROTOBUF: $ENV{CMAKE_USE_SYSTEM_PROTOBUF}")
message(STATUS "CMAKE_USE_SYSTEM_BOOST: $ENV{CMAKE_USE_SYSTEM_BOOST}")
