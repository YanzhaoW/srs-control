#include "srs/converters/StructDeserializer.hpp"
#include "srs/converters/StructSerializer.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <gtest/gtest.h>
#include <random>
#include <ranges>

using srs::StructData;

namespace process = srs::process;

namespace
{

    auto generate_random_struct_data() -> StructData
    {
        static constexpr auto MAX_NUM_SIZE = 100;
        auto struct_data = StructData{};

        auto random_device = std::random_device{};
        auto random_gen = std::mt19937_64{ random_device };

        auto data_size_gen = std::uniform_int_distribution<uint32_t>{ 1, MAX_NUM_SIZE };

        const auto marker_size = data_size_gen(random_gen);
        const auto hit_size = data_size_gen(random_gen);

        struct_data.header.frame_counter = 1;
        struct_data.header.vmm_tag = std::array<char, 3>{ 'V', 'M', '3' };
        struct_data.header.udp_timestamp = std::chrono::system_clock::now().time_since_epoch().count();

        // NOLINTBEGIN (cppcoreguidelines-avoid-magic-numbers)
        auto vmm_id_gen = std::uniform_int_distribution<uint8_t>{ 0, (1U << 5U) - 1 };
        auto srs_timestamp_gen = std::uniform_int_distribution<uint64_t>{ 0, (1U << 42U) - 1 };

        struct_data.marker_data.reserve(marker_size);
        for (auto idx : std::views::iota(uint32_t{ 0 }, marker_size))
        {
            auto& marker = struct_data.marker_data.emplace_back();
            marker.vmm_id = vmm_id_gen(random_gen);
            marker.srs_timestamp = srs_timestamp_gen(random_gen);
        }

        auto channel_num_gen = std::uniform_int_distribution<uint8_t>{ 0, (1U << 6U) - 1 };
        auto tdc_gen = std::uniform_int_distribution<uint8_t>{ 0, (1U << 8U) - 1 };
        auto offset_gen = std::uniform_int_distribution<uint8_t>{ 0, (1U << 5U) - 1 };
        auto adc_gen = std::uniform_int_distribution<uint16_t>{ 0, (1U << 10U) - 1 };
        auto bc_id_gen = std::uniform_int_distribution<uint16_t>{ 0, (1U << 12U) - 1 };
        // NOLINTEND (cppcoreguidelines-avoid-magic-numbers)

        struct_data.hit_data.reserve(hit_size);
        for (auto idx : std::views::iota(uint32_t{ 0 }, hit_size))
        {
            auto& hit = struct_data.hit_data.emplace_back();
            hit.channel_num = channel_num_gen(random_gen);
            hit.channel_num = channel_num_gen(random_gen);
            hit.tdc = tdc_gen(random_gen);
            hit.offset = offset_gen(random_gen);
            hit.adc = adc_gen(random_gen);
            hit.bc_id = bc_id_gen(random_gen);
        }

        return struct_data;
    }

} // namespace

TEST(data_structure, check_de_serialization)
{
    auto random_data = generate_random_struct_data();

    auto serializer_converter = process::StructSerializer();
    auto deserializer_converter = process::StructDeserializer();
}
