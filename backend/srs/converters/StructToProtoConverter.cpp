#include "StructToProtoConverter.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/data/message.pb.h"
#include <cassert>

namespace srs::process
{
    namespace
    {
        void set_header(const StructData& struct_data, proto::Data& output_data)
        {
            const auto& input_header = struct_data.header;
            auto* header = output_data.mutable_header();
            assert(header != nullptr);
            header->set_frame_counter(input_header.frame_counter);
            header->set_fec_id(input_header.fec_id);
            header->set_udp_timestamp(input_header.udp_timestamp);
            header->set_overflow(input_header.overflow);
        }

        void set_marker_data(const StructData& struct_data, proto::Data& output_data)
        {
            const auto& input_marker_data = struct_data.marker_data;
            for (const auto& input_data : input_marker_data)
            {
                auto* marker_data = output_data.add_marker_data();
                marker_data->set_vmm_id(input_data.vmm_id);
                marker_data->set_srs_timestamp(input_data.srs_timestamp);
            }
        }

        void set_hit_data(const StructData& struct_data, proto::Data& output_data)
        {
            const auto& input_hit_data = struct_data.hit_data;
            for (const auto& input_data : input_hit_data)
            {
                auto* hit_data = output_data.add_hit_data();
                hit_data->set_is_over_threshold(input_data.is_over_threshold);
                hit_data->set_channel_num(input_data.channel_num);
                hit_data->set_tdc(input_data.tdc);
                hit_data->set_offset(input_data.offset);
                hit_data->set_vmm_id(input_data.vmm_id);
                hit_data->set_adc(input_data.adc);
                hit_data->set_bc_id(input_data.bc_id);
            }
        }

    } // namespace

    void Struct2ProtoConverter::convert(const StructData& struct_data, proto::Data& output_data)
    {
        set_header(struct_data, output_data);
        set_marker_data(struct_data, output_data);
        set_hit_data(struct_data, output_data);
    }
} // namespace srs::process
