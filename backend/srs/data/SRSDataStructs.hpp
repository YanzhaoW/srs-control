#pragma once

#include <array>
#include <cstdint>
#include <vector>

#ifdef HAS_ROOT
#include <Rtypes.h>
#endif

namespace srs
{
    constexpr auto VMM_TAG_BIT_LENGTH = 3;
    struct ReceiveDataHeader
    {
        uint32_t frame_counter{};                       //!< The counting value for current UDP data frame.
        std::array<char, VMM_TAG_BIT_LENGTH> vmm_tag{}; //!< Hard-coded text "VM3"
        uint8_t fec_id{};                               //!< FEC ID
        uint32_t udp_timestamp{};                       //!< UDP timestamp
        uint32_t overflow{};                            //!< Overflow value
#ifdef HAS_ROOT
        ClassDefNV(ReceiveDataHeader, 1);
#endif
    };

    struct MarkerData
    {
        uint8_t vmm_id{};         //!< VMM ID for the marker data
        uint64_t srs_timestamp{}; //!< Timestamp value
#ifdef HAS_ROOT
        ClassDefNV(MarkerData, 1);
#endif
    };

    struct HitData
    {
        bool is_over_threshold = false; //!< whether the hit data is over the threshould
        uint8_t channel_num{};          //!< Channel number
        uint8_t tdc{};                  //!< TDC value
        uint8_t offset{};               //!< Offset value
        uint8_t vmm_id{};               //!< VMM ID
        uint16_t adc{};                 //!< ADC value
        uint16_t bc_id{};               //!< BC ID
#ifdef HAS_ROOT
        ClassDefNV(HitData, 1);
#endif
    };

    struct StructData
    {
        ReceiveDataHeader header{};          //!< Header data
        std::vector<MarkerData> marker_data; //!< Marker data
        std::vector<HitData> hit_data;       //!< Hit data
#ifdef HAS_ROOT
        ClassDefNV(StructData, 1);
#endif
    };

    inline void reset_struct_data(StructData& struct_data)
    {
        struct_data.header = ReceiveDataHeader{};
        struct_data.marker_data.clear();
        struct_data.hit_data.clear();
    }

} // namespace srs
