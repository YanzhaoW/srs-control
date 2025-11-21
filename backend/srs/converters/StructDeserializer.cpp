#include "srs/converters/DataConverterBase.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/utils/CommonFunctions.hpp"
// #include "srs/workflow/TaskDiagram.hpp"
#include "StructDeserializer.hpp"
#include <algorithm>
#include <bitset>
#include <boost/asio/any_io_executor.hpp>
#include <cstddef>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string_view>
#include <zpp_bits.h>

namespace srs::process
{
    namespace
    {
        using DataElementType = StructDeserializer::DataElementType;

        template <typename T>
        auto convert_to(const DataElementType& raw_data) -> T
        {
            constexpr auto struct_size = sizeof(uint64_t);
            static_assert(common::HIT_DATA_BIT_LENGTH <= common::BYTE_BIT_LENGTH * struct_size);
            static_assert(sizeof(T) == struct_size);
            auto expanded_raw_data = std::bitset<common::BYTE_BIT_LENGTH * struct_size>(raw_data.to_ullong());
            constexpr auto shifted_bits = (struct_size * common::BYTE_BIT_LENGTH) - common::HIT_DATA_BIT_LENGTH;
            expanded_raw_data = expanded_raw_data << shifted_bits;
            return std::bit_cast<T>(expanded_raw_data.to_ullong());
        }
        struct HitDataCompact
        {
            uint16_t : 16;
            uint16_t tdc : 8;
            uint16_t channel_num : 6;
            uint16_t is_over_threshold : 1;
            uint16_t flag : 1;
            uint32_t bc_id : 12;
            uint32_t adc : 10;
            uint32_t vmm_id : 5;
            uint32_t offset : 5;
        };

        struct MarkerDataCompact
        {
            uint16_t : 16;
            uint16_t timestamp_low_bits : common::SRS_TIMESTAMP_LOW_BIT_LENGTH;
            uint16_t vmm_id : 5;
            uint16_t flag : 1;
            uint32_t timestamp_high_bits : common::SRS_TIMESTAMP_HIGH_BIT_LENGTH;
        };

        void array_to_marker(const DataElementType& raw_data, MarkerData& marker_data)
        {
            auto marker_data_compact = convert_to<MarkerDataCompact>(raw_data);

            auto timestamp_high_bits = std::bitset<common::SRS_TIMESTAMP_HIGH_BIT_LENGTH>(
                static_cast<uint32_t>(marker_data_compact.timestamp_high_bits));
            auto timestamp_low_bits = std::bitset<common::SRS_TIMESTAMP_LOW_BIT_LENGTH>(
                static_cast<uint32_t>(marker_data_compact.timestamp_low_bits));
            marker_data.srs_timestamp = static_cast<decltype(marker_data.srs_timestamp)>(
                common::merge_bits(timestamp_high_bits, timestamp_low_bits).to_ullong());
            marker_data.vmm_id = static_cast<decltype(marker_data.vmm_id)>(marker_data_compact.vmm_id);
        }

        void array_to_hit(const DataElementType& raw_data, HitData& hit_data)
        {
            auto hit_data_compact = convert_to<HitDataCompact>(raw_data);

            hit_data.is_over_threshold =
                static_cast<decltype(hit_data.is_over_threshold)>(hit_data_compact.is_over_threshold);
            hit_data.channel_num = static_cast<decltype(hit_data.channel_num)>(hit_data_compact.channel_num);
            hit_data.tdc = static_cast<decltype(hit_data.tdc)>(hit_data_compact.tdc);
            hit_data.offset = static_cast<decltype(hit_data.offset)>(hit_data_compact.offset);
            hit_data.vmm_id = static_cast<decltype(hit_data.vmm_id)>(hit_data_compact.vmm_id);
            hit_data.adc = static_cast<decltype(hit_data.adc)>(hit_data_compact.adc);
            hit_data.bc_id = static_cast<decltype(hit_data.bc_id)>(hit_data_compact.bc_id);
            hit_data.bc_id = common::gray_to_binary(hit_data.bc_id);
        }
    }; // namespace

    StructDeserializer::StructDeserializer(size_t n_lines)
        : ConverterTask{ "Struct deserializer", raw, n_lines }
    {
        receive_raw_data_.resize(n_lines);
        output_data_.resize(n_lines);
    }

    // thread safe
    auto StructDeserializer::convert(std::string_view binary_data,
                                     StructData& output_data,
                                     ReceiveDataSquence& receive_raw_data) -> std::size_t
    {
        auto deserialize_to = zpp::bits::in{ binary_data, zpp::bits::endian::network{}, zpp::bits::no_size{} };

        auto read_bytes = binary_data.size() * sizeof(BufferElementType);
        constexpr auto header_bytes = sizeof(output_data.header);
        constexpr auto element_bytes = common::HIT_DATA_BIT_LENGTH / common::BYTE_BIT_LENGTH;
        auto vector_size = (read_bytes - header_bytes) / element_bytes;
        if (vector_size < 0)
        {
            throw std::runtime_error("Deserialization: Wrong header type!");
        }

        // locking mutex to prevent data racing
        receive_raw_data.resize(vector_size);
        std::ranges::fill(receive_raw_data, 0);
        deserialize_to(output_data.header, receive_raw_data).or_throw();
        byte_reverse_data_sq(receive_raw_data);
        translate_raw_data(output_data, receive_raw_data);

        return receive_raw_data.size();
    }

    void StructDeserializer::translate_raw_data(StructData& struct_data, ReceiveDataSquence& receive_raw_data)
    {
        for (const auto& element : receive_raw_data)
        {
            // spdlog::info("raw data: {:x}", element.to_ullong());
            if (auto is_hit = check_is_hit(element); is_hit)
            {
                auto& hit_data = struct_data.hit_data.emplace_back();
                array_to_hit(element, hit_data);
            }
            else
            {
                auto& marker_data = struct_data.marker_data.emplace_back();
                array_to_marker(element, marker_data);
            }
        }
    }
    void StructDeserializer::byte_reverse_data_sq(ReceiveDataSquence& receive_raw_data)
    {
        // reverse byte order of each element.
        // TODO: This is current due to a bug from zpp_bits. The reversion is not needed if it's fixed.
        for (auto& element : receive_raw_data)
        {
            element = common::byte_swap(element);
        }
    }

} // namespace srs::process
