target_sources(
    srscpp
    PRIVATE
        ConnectionBase.cpp
        Connections.cpp
        DataSocket.cpp
        FecSwitchSocket.cpp
        SpecialSocketBase.cpp
)

target_sources(
    srscpp
    PRIVATE
        FILE_SET privateHeaders
            FILES
                ConnectionBase.hpp
                Connections.hpp
                ConnectionTypeDef.hpp
                DataSocket.hpp
                FecSwitchSocket.hpp
                SpecialSocketBase.hpp
)
