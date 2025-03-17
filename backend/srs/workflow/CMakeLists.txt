target_sources(srscpp PRIVATE Handler.cpp TaskDiagram.cpp)

target_link_libraries(srscpp PRIVATE $<BUILD_LOCAL_INTERFACE:TBB::tbb>)

target_sources(
    srscpp
    PRIVATE FILE_SET
            privateHeaders
            FILES
            TaskDiagram.hpp
            Handler.hpp)
