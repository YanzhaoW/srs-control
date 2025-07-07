target_sources(srscpp PRIVATE Connections.cpp)

target_sources(
    srscpp
    PRIVATE FILE_SET
            privateHeaders
            FILES
            ConnectionBase.hpp
            ConnectionTypeDef.hpp
            Connections.hpp)
