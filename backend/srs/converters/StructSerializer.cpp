#include "StructSerializer.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/data/SRSDataCompact.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <array>
#include <bit>
#include <bitset>
#include <boost/asio/any_io_executor.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <spdlog/spdlog.h>
#include <string_view>
#include <type_traits>
#include <vector>
#include <zpp_bits.h>

namespace srs::process
{
    namespace
    {
        template <class T>
        auto check_compact_max_size(const T& value, size_t length) -> std::expected<void, std::string_view>
        {
            if (value >= 1U << length)
            {
                return std::unexpected("Serialization: exceeding size limit!");
            }
            return {};
        }

        auto marker_to_compact(const MarkerData& marker_data, internal::MarkerDataCompact& marker_data_compact)
            -> std::expected<void, std::string_view>
        {
            auto is_correct_sizes =
                check_compact_max_size(marker_data.vmm_id, internal::VMM_ID_BIT_LENGTH)
                    .and_then(
                        [&]
                        {
                            return check_compact_max_size(
                                marker_data.srs_timestamp,
                                common::SRS_TIMESTAMP_LOW_BIT_LENGTH + common::SRS_TIMESTAMP_HIGH_BIT_LENGTH);
                        });
            if (!is_correct_sizes)
            {
                return std::unexpected(is_correct_sizes.error());
            }
            auto timestamp_bits =
                std::bitset<common::SRS_TIMESTAMP_HIGH_BIT_LENGTH + common::SRS_TIMESTAMP_LOW_BIT_LENGTH>(
                    marker_data.srs_timestamp);
            auto [timestamp_low_bits, timestamp_high_bits] =
                common::split_bits<common::SRS_TIMESTAMP_LOW_BIT_LENGTH>(timestamp_bits);
            marker_data_compact.timestamp_low_bits =
                static_cast<decltype(marker_data_compact.timestamp_low_bits)>(timestamp_low_bits.to_ulong());
            marker_data_compact.timestamp_high_bits =
                static_cast<decltype(marker_data_compact.timestamp_high_bits)>(timestamp_high_bits.to_ulong());
            marker_data_compact.flag = static_cast<decltype(marker_data_compact.flag)>(0);
            marker_data_compact.vmm_id = static_cast<decltype(marker_data_compact.vmm_id)>(marker_data.vmm_id);
            return {};
        }

        auto hit_to_compact(const HitData& hit_data, internal::HitDataCompact& hit_data_compact)
            -> std::expected<void, std::string_view>
        {
            auto is_correct_sizes =
                check_compact_max_size(hit_data.channel_num, internal::CHANNEL_NUM_BIT_LENGTH)
                    .and_then([&] { return check_compact_max_size(hit_data.bc_id, internal::BC_ID_BIT_LENGTH); })
                    .and_then([&] { return check_compact_max_size(hit_data.adc, internal::ADC_BIT_LENGTH); })
                    .and_then([&] { return check_compact_max_size(hit_data.vmm_id, internal::VMM_ID_BIT_LENGTH); })
                    .and_then([&] { return check_compact_max_size(hit_data.offset, internal::OFFSET_BIT_LENGTH); });
            if (!is_correct_sizes)
            {
                return std::unexpected(is_correct_sizes.error());
            }
            hit_data_compact.vmm_id = static_cast<decltype(hit_data_compact.vmm_id)>(hit_data.vmm_id);
            hit_data_compact.flag = static_cast<decltype(hit_data_compact.flag)>(1);
            hit_data_compact.adc = static_cast<decltype(hit_data_compact.adc)>(hit_data.adc);
            hit_data_compact.bc_id = static_cast<decltype(hit_data_compact.bc_id)>(hit_data.bc_id);
            hit_data_compact.bc_id = common::binary_to_gray(hit_data_compact.bc_id);
            hit_data_compact.channel_num = static_cast<decltype(hit_data_compact.channel_num)>(hit_data.channel_num);
            hit_data_compact.is_over_threshold =
                static_cast<decltype(hit_data_compact.is_over_threshold)>(hit_data.is_over_threshold);
            hit_data_compact.offset = static_cast<decltype(hit_data_compact.offset)>(hit_data.offset);
            return {};
        }

        template <class T>
            requires(std::same_as<std::remove_cvref_t<T>, internal::HitDataCompact> ||
                     std::same_as<std::remove_cvref_t<T>, internal::MarkerDataCompact>)
        auto compact_to_vector(const T& compact_data) -> std::vector<char>
        {
            auto compact_bitset =
                std::bitset<sizeof(std::uint64_t) * common::BYTE_BIT_LENGTH>{ std::bit_cast<uint64_t>(compact_data) };
            auto output = std::vector<char>{};
            output.resize(sizeof(std::uint64_t) / common::BYTE_BIT_LENGTH);
            auto write_to_output = zpp::bits::out{ output, zpp::bits::endian::network{}, zpp::bits::no_size{} };
            write_to_output(compact_bitset).or_throw();
            output.resize(common::HIT_DATA_BIT_LENGTH / common::BYTE_BIT_LENGTH);
            return output;
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
        for (auto hit : input->hit_data)
        {
            auto hit_compact = internal::HitDataCompact{};
            if (auto r = hit_to_compact(hit, hit_compact); !r)
            {
                return std::unexpected(r.error());
            }
            serialize_to(compact_to_vector(hit_compact)).or_throw();
        }
        for (auto marker : input->marker_data)
        {
            auto marker_compact = internal::MarkerDataCompact{};
            if (auto r = marker_to_compact(marker, marker_compact); !r)
            {
                return std::unexpected(r.error());
            }
            serialize_to(compact_to_vector(marker_compact)).or_throw();
        }
        return input->marker_data.size() + input->hit_data.size();
    }
    // NOLINTEND
} // namespace srs::process
