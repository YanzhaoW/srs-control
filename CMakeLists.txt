cmake_minimum_required(VERSION 3.28)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(option_settings)

set(ENV{CMAKE_ENABLE_TEST} ${ENABLE_TEST})

project(
    SRS-control
    VERSION 0.1.3
    LANGUAGES CXX C)

if(BUILD_ONLY_DOC)
    add_subdirectory(doc)
else()
    if(BUILD_DOC)
        add_subdirectory(doc)
    endif()
    include(install_config)
    include(project_config)
    include(check_compilers)

    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_EXTENSIONS ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        list(APPEND CMAKE_CXX_FLAGS "-fexperimental-library")
    endif()

    include(import_packages)

    add_subdirectory(frontend)
    add_subdirectory(backend)
    add_subdirectory(examples)
    if(ENABLE_TEST)
        add_subdirectory(test)
    endif()
endif()
