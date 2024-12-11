#pragma once

#include <srs/data/SRSDataStructs.hpp>
#include <srs/data/message.pb.h>

namespace srs::process
{
    class Proto2StructConverter
    {
      public:
        Proto2StructConverter() = default;

        static void convert(const proto::Data& proto, StructData& struct_data)
        {
            reset_struct_data(struct_data);

            set_header(proto, struct_data);
            set_hit_data(proto, struct_data);
            set_marker_data(proto, struct_data);
        }

        auto convert(const proto::Data& proto) -> const StructData&
        {
            convert(proto, data_);
            return data_;
        }

      private:
        StructData data_;

        static void set_header(const proto::Data& proto, StructData& struct_data)
        {
            const auto& proto_header = proto.header();

            auto& header = struct_data.header;
            header.frame_counter = proto_header.frame_counter();
            header.fec_id = proto_header.fec_id();
            header.udp_timestamp = proto_header.udp_timestamp();
            header.overflow = proto_header.overflow();
        }

        static void set_marker_data(const proto::Data& proto, StructData& struct_data)
        {
            const auto& proto_marker_data = proto.marker_data();
            struct_data.marker_data.reserve(proto_marker_data.size());

            for (const auto& proto_marker : proto_marker_data)
            {
                auto& marker_data = struct_data.marker_data.emplace_back();
                marker_data.srs_timestamp = proto_marker.srs_timestamp();
                marker_data.vmm_id = static_cast<uint8_t>(proto_marker.vmm_id());
            }
        }

        static void set_hit_data(const proto::Data& proto, StructData& struct_data)
        {
            const auto& proto_hit_data = proto.hit_data();
            struct_data.hit_data.reserve(proto_hit_data.size());

            for (const auto& proto_hit : proto_hit_data)
            {
                auto& hit_data = struct_data.hit_data.emplace_back();

                hit_data.is_over_threshold = proto_hit.is_over_threshold();
                hit_data.channel_num = proto_hit.channel_num();
                hit_data.tdc = proto_hit.tdc();
                hit_data.offset = proto_hit.offset();
                hit_data.vmm_id = proto_hit.vmm_id();
                hit_data.adc = proto_hit.adc();
                hit_data.bc_id = proto_hit.bc_id();
            }
        }
    };

} // namespace srs
