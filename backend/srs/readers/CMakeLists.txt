target_sources(
    srscpp
    PRIVATE ProtoMsgReader.cpp RawFrameReader.cpp
    PUBLIC FILE_SET publicHeaders FILES ProtoMsgReader.hpp RawFrameReader.hpp
)
