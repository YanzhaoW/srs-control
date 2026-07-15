#pragma once

#include <cstdint>
#include <srs/utils/CommonDefinitions.hpp>

namespace srs::process::internal
{
    // Dev note: need to be of unsigned type to stop clang tidy from complaining
    constexpr auto CHANNEL_NUM_BIT_LENGTH = 6U;
    constexpr auto TDC_BIT_LENGTH = 8U;
    constexpr auto BC_ID_BIT_LENGTH = 12U;
    constexpr auto ADC_BIT_LENGTH = 10U;
    constexpr auto VMM_ID_BIT_LENGTH = 5U;
    constexpr auto OFFSET_BIT_LENGTH = 5U;

    constexpr auto CHANNEL_NUM_BIT_MAX = (1ULL << CHANNEL_NUM_BIT_LENGTH) - 1;
    constexpr auto TDC_BIT_MAX = (1ULL << TDC_BIT_LENGTH) - 1;
    constexpr auto OFFSET_BIT_MAX = (1ULL << OFFSET_BIT_LENGTH) - 1;
    constexpr auto VMM_ID_BIT_MAX = (1ULL << VMM_ID_BIT_LENGTH) - 1;
    constexpr auto ADC_BIT_MAX = (1ULL << ADC_BIT_LENGTH) - 1;
    constexpr auto BC_ID_BIT_MAX = (1ULL << BC_ID_BIT_LENGTH) - 1;
    constexpr auto SRS_TIMESTAMP_MAX =
        (1ULL << (common::SRS_TIMESTAMP_LOW_BIT_LENGTH + common::SRS_TIMESTAMP_HIGH_BIT_LENGTH)) - 1;

    struct HitDataCompact
    {
        uint16_t : 16;
        uint16_t tdc : TDC_BIT_LENGTH;
        uint16_t channel_num : CHANNEL_NUM_BIT_LENGTH;
        uint16_t is_over_threshold : 1;
        uint16_t flag : 1;
        uint32_t bc_id : BC_ID_BIT_LENGTH;
        uint32_t adc : ADC_BIT_LENGTH;
        uint32_t vmm_id : VMM_ID_BIT_LENGTH;
        uint32_t offset : OFFSET_BIT_LENGTH;
    };

    struct MarkerDataCompact
    {
        uint16_t : 16;
        uint16_t timestamp_low_bits : common::SRS_TIMESTAMP_LOW_BIT_LENGTH;
        uint16_t vmm_id : OFFSET_BIT_LENGTH;
        uint16_t flag : 1;
        uint32_t timestamp_high_bits : common::SRS_TIMESTAMP_HIGH_BIT_LENGTH;
    };
} // namespace srs::process::internal
