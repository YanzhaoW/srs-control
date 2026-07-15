include(FetchContent)

FetchContent_Declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG v7.9.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glaze)

find_package(asio REQUIRED CONFIG)
find_package(fmt REQUIRED CONFIG)
find_package(zpp_bits REQUIRED CONFIG)
find_package(gsl-lite REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(CLI11 REQUIRED CONFIG)
find_package(concurrentqueue REQUIRED)
find_package(magic_enum REQUIRED CONFIG)
find_package(Taskflow REQUIRED CONFIG)

# find_package(glaze REQUIRED CONFIG)
find_package(ctre REQUIRED CONFIG)

# set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(protobuf CONFIG COMPONENTS libprotobuf protoc)

if(NOT protobuf_FOUND)
    message(STATUS "Trying to find Protobuf pakcage with module mode.")
    find_package(Protobuf MODULE REQUIRED COMPONENTS libprotobuf protoc)
else()
    message(STATUS "Couldn't find protobuf with config mode.")
endif()

if(ENABLE_TEST)
    find_package(Catch2 CONFIG REQUIRED)
endif()

if(NOT NO_ROOT)
    if(USE_ROOT)
        message(STATUS "Force to use ROOT depenedency!")
        set(CMAKE_MESSAGE_LOG_LEVEL ERROR) # turn off annoying root warnings
        find_package(ROOT QUIET REQUIRED COMPONENTS RIO Tree Hist Gpad)
        set(CMAKE_MESSAGE_LOG_LEVEL STATUS)
    else()
        set(CMAKE_MESSAGE_LOG_LEVEL ERROR)
        find_package(ROOT QUIET COMPONENTS RIO Tree Hist Gpad)
        set(CMAKE_MESSAGE_LOG_LEVEL STATUS)
    endif()
endif()

if(ROOT_FOUND)
    message(STATUS "ROOT depenedency is enabled!")
else()
    message(STATUS "ROOT depenedency is disabled!")
endif()

set(PRIVATE_THIRD_PARTY_LIBRARIES
    $<BUILD_LOCAL_INTERFACE:asio::asio>
    $<BUILD_LOCAL_INTERFACE:fmt::fmt-header-only>
    $<BUILD_LOCAL_INTERFACE:gsl::gsl-lite>
    $<BUILD_LOCAL_INTERFACE:magic_enum::magic_enum>
    $<BUILD_LOCAL_INTERFACE:spdlog::spdlog>
    $<BUILD_LOCAL_INTERFACE:Taskflow::Taskflow>
    $<BUILD_LOCAL_INTERFACE:zpp_bits::zpp_bits>
    $<BUILD_LOCAL_INTERFACE:concurrentqueue::concurrentqueue>
)

set(THIRD_PARTY_LIBRARIES
    CLI11::CLI11
    asio::asio
    glaze::glaze
    magic_enum::magic_enum
    spdlog::spdlog
)
