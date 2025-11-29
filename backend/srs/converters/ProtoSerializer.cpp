#include "ProtoSerializer.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include <google/protobuf/io/gzip_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/util/delimited_message_util.h>

namespace srs::process
{
    auto protobuf_delim_deserializer_converter::operator()(const proto::Data& proto_data, std::string& output_data)
        -> int
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

    auto protobuf_deserializer_converter::operator()(const proto::Data& proto_data, std::string& output_data) -> int
    {
        namespace protobuf = google::protobuf;
        namespace io = protobuf::io;
        auto output_stream = io::StringOutputStream{ &output_data };
        proto_data.SerializeToZeroCopyStream(&output_stream);
        return 0;
    };
} // namespace srs::process
