target_sources(srscpp PRIVATE ProtoMsgReader.cpp RawFrameReader.cpp)
target_sources(
    srscpp
    PUBLIC FILE_SET publicHeaders FILES ProtoMsgReader.hpp RawFrameReader.hpp
)
