#include "StructSerializer.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/data/SRSDataCompact.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <algorithm>
#include <bitset>
#include <boost/asio/any_io_executor.hpp>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <zpp_bits.h>

namespace srs::process
{
    namespace
    {
        void marker_to_compact(const MarkerData& marker_data, internal::MarkerDataCompact& marker_data_compact)
        {
            auto timestamp_bits = std::bitset<common::SRS_TIMESTAMP_HIGH_BIT_LENGTH + common::SRS_TIMESTAMP_LOW_BIT_LENGTH>(marker_data.srs_timestamp);
            auto timestamp_low_bits = std::bitset<common::SRS_TIMESTAMP_LOW_BIT_LENGTH>{};
            auto timestamp_high_bits = std::bitset<common::SRS_TIMESTAMP_HIGH_BIT_LENGTH>{};
            common::split_bits(timestamp_bits, timestamp_high_bits, timestamp_low_bits);
            marker_data_compact.flag = static_cast<decltype(marker_data_compact.flag)>(0);
            marker_data_compact.vmm_id = static_cast<decltype(marker_data_compact.vmm_id)>(marker_data.vmm_id);
        }

        void hit_to_compact(const HitData& hit_data, internal::HitDataCompact& hit_data_compact)
        {
            hit_data_compact.vmm_id = static_cast<decltype(hit_data_compact.vmm_id)>(hit_data.vmm_id);
            hit_data_compact.flag = static_cast<decltype(hit_data_compact.flag)>(1);
            hit_data_compact.adc = static_cast<decltype(hit_data_compact.adc)>(hit_data.adc);
            hit_data_compact.bc_id = static_cast<decltype(hit_data_compact.bc_id)>(hit_data.bc_id);
            hit_data_compact.bc_id = common::bin_to_gray(hit_data_compact.bc_id);
            hit_data_compact.channel_num = static_cast<decltype(hit_data_compact.channel_num)>(hit_data.channel_num);
            hit_data_compact.is_over_threshold = static_cast<decltype(hit_data_compact.is_over_threshold)>(hit_data.is_over_threshold);
            hit_data_compact.offset = static_cast<decltype(hit_data_compact.offset)>(hit_data.offset);
        }
    }; // namespace

    StructSerializer::StructSerializer(size_t n_lines)
        : ConverterTask{ "Struct deserializer", none, n_lines }
    {
        output_data_.resize(n_lines);
    }

    // NOLINTBEGIN
    auto StructSerializer::convert([[maybe_unused]] const StructData* input, [[maybe_unused]] std::vector<char>& output)
        -> std::expected<std::size_t, std::string_view>
    {
        auto serialize_to = zpp::bits::out{ output, zpp::bits::endian::network{}, zpp::bits::no_size{} };
        return 0;
    }
    // NOLINTEND
} // namespace srs::process
