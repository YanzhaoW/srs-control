#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/ProtoSerializerBase.hpp"
#include "srs/data/message.pb.h"
#include "srs/utils/CommonDefinitions.hpp"
#include <array>
#include <cstddef>
#include <google/protobuf/io/gzip_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/util/delimited_message_util.h>
#include <string>
#include <string_view>

namespace srs::process
{
    const auto protobuf_delim_deserializer_converter = [](const proto::Data& proto_data,
                                                          std::string& output_data) -> int
    {
        namespace protobuf = google::protobuf;
        namespace io = protobuf::io;
        auto output_stream = io::StringOutputStream{ &output_data };

        if constexpr (common::PROTOBUF_ENABLE_GZIP)
        {
            auto option = io::GzipOutputStream::Options{};
            option.compression_level = common::GZIP_DEFAULT_COMPRESSION_LEVEL;
            auto gzip_output = io::GzipOutputStream{ &output_stream, option };
            protobuf::util::SerializeDelimitedToZeroCopyStream(proto_data, &gzip_output);
            gzip_output.Flush();
        }
        else
        {
            protobuf::util::SerializeDelimitedToZeroCopyStream(proto_data, &output_stream);
        }
        return 0;
    };

    class ProtoDelimSerializer
        : public ProtoSerializerBase<decltype(protobuf_delim_deserializer_converter), DataConvertOptions::proto_frame>
    {
      public:
        explicit ProtoDelimSerializer(std::size_t n_lines)
            : ProtoSerializerBase{ "ProtoDelimSerializer", protobuf_delim_deserializer_converter, n_lines }
        {
        }
        static constexpr auto ConverterOptions = std::array{ proto_frame };
        static constexpr auto RequiredConverterOptions = std::array{ structure_to_proto };
        static constexpr auto name_ = std::string_view{ "Struct deserializer(delim)" };
        static constexpr auto get_name() -> std::string_view { return name_; };
        static auto get_name_str() -> std::string { return std::string{ name_ }; };
    };
} // namespace srs::process
