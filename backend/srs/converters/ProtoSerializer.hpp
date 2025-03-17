#pragma once

#include <srs/converters/ProtoSerializerBase.hpp>

namespace srs::process
{
    const auto protobuf_deserializer_converter = [](const proto::Data& proto_data, std::string& output_data) -> int
    {
        namespace protobuf = google::protobuf;
        namespace io = protobuf::io;
        auto output_stream = io::StringOutputStream{ &output_data };
        proto_data.SerializeToZeroCopyStream(&output_stream);
        return 0;
    };

    class ProtoSerializer : public ProtoSerializerBase<decltype(protobuf_deserializer_converter)>
    {
      public:
        explicit ProtoSerializer(asio::thread_pool& thread_pool)
            : ProtoSerializerBase{ thread_pool, "ProtoSerializer", protobuf_deserializer_converter }
        {
        }
        static constexpr auto ConverterOption = std::array{ proto };
    };
} // namespace srs
