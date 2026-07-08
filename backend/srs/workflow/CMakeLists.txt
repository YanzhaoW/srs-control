target_sources(
    srscpp
    PRIVATE Handler.cpp TaskDiagram.cpp
    PRIVATE FILE_SET privateHeaders FILES TaskDiagram.hpp Handler.hpp
)
