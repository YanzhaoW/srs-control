#pragma once

#include <cstdint>
#include <srs/utils/CommonDefinitions.hpp>

namespace srs::process::internal {
    
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
}
