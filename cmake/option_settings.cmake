option(USE_ROOT "Force to use ROOT dependency." OFF)
option(NO_ROOT "Disable the usage of ROOT dependency." OFF)
option(BUILD_STATIC "Enable static linking of libstdc++." OFF)
option(ENABLE_TEST "Enable testing framework of the project." ON)

set(SPHINX_BUILDER
    "html"
    CACHE STRING "Set the builder for sphnix-build.")
