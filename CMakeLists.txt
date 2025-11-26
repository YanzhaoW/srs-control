cmake_minimum_required(VERSION 3.28)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(option_settings)
include(check_submodules)

project(
    SRS-control
    VERSION 0.1.4
    LANGUAGES CXX C)

include(CTest)
enable_testing()

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

    include(import_packages)

    add_subdirectory(frontend)
    add_subdirectory(backend)
    add_subdirectory(examples)
    if(ENABLE_TEST)
        add_subdirectory(test)
    endif()
endif()

include(project_packaging)
