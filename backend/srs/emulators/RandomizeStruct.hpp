#pragma once

#include "srs/data/SRSDataCompact.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <random>
#include <ranges>
#include <ratio>
#include <vector>

namespace srs::emulator
{
    class Randomizer
    {
      public:
        struct Config
        {
            common::MinMax<std::size_t> n_hits{};
            common::MinMax<std::size_t> n_markers{};
        };

        Randomizer(const Config& config)
            : config_{ config }
        {
        }

        struct Opt
        {
            uint32_t frame_counter{};
            uint8_t fec_id{};
        };

        auto randomize_data_struct(StructData& data, const Opt& option)
        {
            set_header(data.header, option.frame_counter, option.fec_id);
            randomize_hits(data.hit_data);
            randomize_markers(data.marker_data);
        }

      private:
        std::random_device device_{};
        std::mt19937 rand_gen_{ device_() };
        Config config_;
        std::uniform_int_distribution<std::size_t> n_hits_gen_{ config_.n_hits.min, config_.n_hits.max };
        std::uniform_int_distribution<std::size_t> n_markers_gen_{ config_.n_markers.min, config_.n_markers.max };
        std::uniform_int_distribution<uint8_t> channel_num_gen_{ 0, process::internal::CHANNEL_NUM_BIT_MAX };
        std::uniform_int_distribution<uint8_t> tdc_gen_{ 0, process::internal::TDC_BIT_MAX };
        std::uniform_int_distribution<uint8_t> offset_gen_{ 0, process::internal::OFFSET_BIT_MAX };
        std::uniform_int_distribution<uint8_t> vmm_id_gen_{ 0, process::internal::VMM_ID_BIT_MAX };
        std::uniform_int_distribution<uint16_t> adc_gen_{ 0, process::internal::ADC_BIT_MAX };
        std::uniform_int_distribution<uint16_t> bc_id_gen_{ 0, process::internal::BC_ID_BIT_MAX };

        std::uniform_int_distribution<uint64_t> srs_timestamp_gen_{ 0, process::internal::SRS_TIMESTAMP_MAX };

        void randomize_hits(std::vector<HitData>& hits)
        {
            const auto n_hits = n_hits_gen_(rand_gen_);
            hits.reserve(n_hits);
            hits.clear();

            for (const auto _ : std::views::iota(std::size_t{}, n_hits))
            {
                hits.emplace_back(false,
                                  channel_num_gen_(rand_gen_),
                                  tdc_gen_(rand_gen_),
                                  offset_gen_(rand_gen_),
                                  vmm_id_gen_(rand_gen_),
                                  adc_gen_(rand_gen_),
                                  bc_id_gen_(rand_gen_));
            }
        }

        void randomize_markers(std::vector<MarkerData>& markers)
        {

            const auto n_markers = n_markers_gen_(rand_gen_);
            markers.reserve(n_markers);
            markers.clear();
            for (const auto _ : std::views::iota(std::size_t{}, n_markers))
            {
                markers.emplace_back(vmm_id_gen_(rand_gen_), srs_timestamp_gen_(rand_gen_));
            }
        }

        void set_header(ReceiveDataHeader& header, uint32_t frame_counter, uint8_t fec_id) const
        {
            const auto tp = std::chrono::time_point_cast<std::chrono::duration<uint32_t, std::milli>>(
                std::chrono::system_clock::now());
            header.frame_counter = frame_counter;
            header.vmm_tag = std::array{ 'V', 'M', '3' };
            header.fec_id = fec_id;
            header.udp_timestamp = tp.time_since_epoch().count();
            header.overflow = 0;
        }
    };

} // namespace srs::emulator
