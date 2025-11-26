target_sources(srscpp PRIVATE StructDeserializer.cpp RawToDelimRawConveter.cpp
                              StructToProtoConverter.cpp ProtoSerializer.cpp)

target_sources(
    srscpp
    PRIVATE FILE_SET
            privateHeaders
            FILES
            DataConvertOptions.hpp
            ProtoSerializer.hpp
            SerializableBuffer.hpp
            StructDeserializer.hpp
            StructToProtoConverter.hpp)
