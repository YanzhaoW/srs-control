include(FetchContent)

fetchcontent_declare(
    glaze
    GIT_REPOSITORY https://github.com/stephenberry/glaze.git
    GIT_TAG main
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL)

fetchcontent_makeavailable(glaze)

find_package(Boost REQUIRED CONFIG COMPONENTS thread)
find_package(fmt REQUIRED)
find_package(zpp_bits REQUIRED)
find_package(gsl-lite REQUIRED)
find_package(spdlog REQUIRED)
find_package(CLI11 REQUIRED)
find_package(TBB REQUIRED)
find_package(magic_enum REQUIRED)

set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH OFF)
set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH OFF)
find_package(protobuf CONFIG COMPONENTS libprotobuf)
set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH ON)
set(CMAKE_FIND_USE_CMAKE_SYSTEM_PATH ON)

if(NOT protobuf_FOUND)
    message(STATUS "Find Protobuf pakcage with module mode.")
    find_package(Protobuf MODULE COMPONENTS libprotobuf)
else()
    message(STATUS "Find Protobuf pakcage with config mode.")
endif()

if(ENABLE_TEST)
    find_package(GTest CONFIG REQUIRED)
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
