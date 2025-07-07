target_sources(srscpp PRIVATE StructDeserializer.cpp)

target_sources(
    srscpp
    PRIVATE FILE_SET
            privateHeaders
            FILES
            DataConvertOptions.hpp
            ProtoDelimSerializer.hpp
            ProtoSerializer.hpp
            ProtoSerializerBase.hpp
            SerializableBuffer.hpp
            StructDeserializer.hpp
            StructToProtoConverter.hpp)
