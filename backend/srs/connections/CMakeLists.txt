target_sources(srscpp PRIVATE Connections.cpp FecSwitchSocket.cpp SpecialSocketBase.cpp
                              DataSocket.cpp)

target_sources(
    srscpp
    PRIVATE FILE_SET
            privateHeaders
            FILES
            ConnectionBase.hpp
            ConnectionTypeDef.hpp
            Connections.hpp
            FecSwitchSocket.hpp
            DataSocket.hpp
            SpecialSocketBase.hpp)
