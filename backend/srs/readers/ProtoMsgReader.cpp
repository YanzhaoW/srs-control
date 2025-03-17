#include <srs/converters/ProtoDeserializer.hpp>
#include <srs/converters/ProtoToStructConverter.hpp>
#include <srs/readers/ProtoMsgReader.hpp>

namespace srs::reader
{
    ProtoMsg::ProtoMsg()
        : proto_to_struct_converter_{ std::make_unique<process::Proto2StructConverter>() }
        , proto_deserializer_{ std::make_unique<process::ProtoDeserializer>() }
    {
    }

    ProtoMsg::~ProtoMsg() = default;

    void ProtoMsg::convert(std::string_view msg, StructData& struct_data)
    {
        const auto& prot_struct = proto_deserializer_->convert(msg);
        process::Proto2StructConverter::convert(prot_struct, struct_data);
    }

    auto ProtoMsg::convert(std::string_view msg) -> const StructData&
    {
        const auto& prot_struct = proto_deserializer_->convert(msg);
        return proto_to_struct_converter_->convert(prot_struct);
    }
} // namespace srs
