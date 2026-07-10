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
find_package(re2 REQUIRED CONFIG)
if(USE_SYSTEM_PROTOBUF)
    message(STATUS "Using system protobuf.")
    find_package(protobuf MODULE COMPONENTS libprotobuf)
else()
    message(STATUS "Using protobuf from conan.")
    set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH OFF)
    set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH OFF)
    set(protobuf_MODULE_COMPATIBLE TRUE)
    find_package(protobuf CONFIG COMPONENTS libprotobuf)
    set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH ON)
    set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH ON)
endif()

if(NOT protobuf_FOUND)
    message(STATUS "Find Protobuf pakcage with module mode.")
    find_package(Protobuf MODULE COMPONENTS libprotobuf)
endif()

message(STATUS "Find Protobuf pakcage with config mode.")
message(STATUS "Protobuf version: ${protobuf_VERSION}")
message(STATUS "protoc: ${Protobuf_PROTOC_EXECUTABLE}")

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
