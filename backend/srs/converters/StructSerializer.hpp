#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/data/SRSDataStructs.hpp"
#include "srs/utils/CommonConcepts.hpp"
#include <cstddef>
#include <expected>
#include <string_view>
#include <vector>

namespace srs::process
{
    class StructSerializer : public ConverterTask<DataConvertOptions::structure, const StructData*, std::string_view>
    {
      public:
        explicit StructSerializer(size_t n_lines = 1);

        auto convert(const StructData* input, std::vector<char>& output)
            -> std::expected<std::size_t, std::string_view>;

        auto run(const OutputTo<InputType> auto& prev_data_converter, std::size_t line_number = 0) -> RunResult
        {
            auto res = convert(prev_data_converter(line_number), output_data_[line_number]);
            return res.transform([this, line_number](auto) { return this->operator()(line_number); });
        }

        [[nodiscard]] auto operator()(std::size_t line_number = 0) const -> OutputType
        {
            return std::string_view{ output_data_[line_number].data(), output_data_[line_number].size() };
        }

      private:
        std::vector<std::vector<char>> output_data_;
    };
} // namespace srs::process
