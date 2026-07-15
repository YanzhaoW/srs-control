target_sources(
    srscpp
    PRIVATE FECEmulator.cpp World.cpp
    PRIVATE FILE_SET privateHeaders FILES FECEmulator.hpp World.hpp
)
