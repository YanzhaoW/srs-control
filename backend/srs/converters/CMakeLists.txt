target_sources(
    srscpp
    PRIVATE
        ProtoSerializer.cpp
        RawToDelimRawConveter.cpp
        StructDeserializer.cpp
        StructSerializer.cpp
        StructToProtoConverter.cpp
)

target_sources(
    srscpp
    PRIVATE
        FILE_SET privateHeaders
            FILES
                DataConvertOptions.hpp
                ProtoSerializer.hpp
                SerializableBuffer.hpp
                StructDeserializer.hpp
                StructSerializer.hpp
                StructToProtoConverter.hpp
)
