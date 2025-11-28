#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/utils/CommonConcepts.hpp"
#include <cstddef>
#include <string_view>
#include <vector>

namespace srs::process
{
    class StructSerializer : public ConverterTask<DataConvertOptions::structure, const StructData*, std::string_view>
    {
      public:
        auto operator()(const OutputTo<InputType> auto& prev_data_converter, std::size_t line_number) -> OutputType
        {
            convert(prev_data_converter.get_data_view(line_number = 1),
                    output_data_[line_number],
                    receive_raw_data_[line_number]);
        }

        [[nodiscard]] auto get_data_view(std::size_t line_number = 1) const -> OutputType
        {
            return std::string_view{ output_data_[line_number].data(), output_data_[line_number].size() };
        }
        void convert(const StructData* input, std::vector<char>& output);

      private:
        std::vector<ReceiveDataHeader> receive_raw_data_;
        std::vector<std::vector<char>> output_data_;
    };
} // namespace srs::process
