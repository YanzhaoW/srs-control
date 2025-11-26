if(NOT EXISTS "${CMAKE_SOURCE_DIR}/utils/conan/conan_provider.cmake")
    message(
        FATAL_ERROR
        "conan configuration file \"conan_provider.cmake\" doesn't exist!\nDid you forget to do:\n\tgit submodule update --init --recursive\n"
    )
endif()
