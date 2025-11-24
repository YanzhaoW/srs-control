#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/ProtoSerializerBase.hpp"
#include "srs/data/message.pb.h"
#include <cstddef>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <string>

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

    class ProtoSerializer
        : public ProtoSerializerBase<decltype(protobuf_deserializer_converter), DataConvertOptions::proto>
    {
      public:
        explicit ProtoSerializer(std::size_t n_lines)
            : ProtoSerializerBase{ "ProtoSerializer", protobuf_deserializer_converter, n_lines }
        {
        }
    };
} // namespace srs::process
