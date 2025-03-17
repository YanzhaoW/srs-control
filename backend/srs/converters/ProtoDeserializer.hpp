#pragma once

#include <google/protobuf/message_lite.h>
#include <srs/data/message.pb.h>

namespace srs::process
{
    class ProtoDeserializer
    {
      public:
        ProtoDeserializer() = default;

        static void convert(std::string_view data, proto::Data& proto)
        {
            proto.Clear();
            namespace protobuf = google::protobuf;
            namespace io = protobuf::io;
            auto input_stream = io::ArrayInputStream{ data.data(), static_cast<int>(data.size()) };
            proto.ParseFromZeroCopyStream(&input_stream);
        }

        auto convert(std::string_view str_data) -> const proto::Data&
        {
            convert(str_data, data_);
            return data_;
        }

      private:
        proto::Data data_;
    };
} // namespace srs
