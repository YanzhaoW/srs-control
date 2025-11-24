#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include <array>
#include <cassert>
#include <cstddef>
#include <spdlog/spdlog.h>
#include <srs/converters/DataConverterBase.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace srs::workflow
{
    class TaskDiagram;
}

namespace srs::process
{
    class Raw2DelimRawConverter
        : public ConverterTask<DataConvertOptions::raw_frame, std::string_view, std::string_view>
    {
      public:
        explicit Raw2DelimRawConverter(size_t n_lines = 1)
            : ConverterTask{ "RawDelimiter", raw, n_lines }
        {
            output_data_.resize(n_lines);
        }

        Raw2DelimRawConverter(const Raw2DelimRawConverter&) = default;
        Raw2DelimRawConverter(Raw2DelimRawConverter&&) = delete;
        Raw2DelimRawConverter& operator=(const Raw2DelimRawConverter&) = default;
        Raw2DelimRawConverter& operator=(Raw2DelimRawConverter&&) = delete;
        ~Raw2DelimRawConverter() { spdlog::debug("Taskflow: raw data delimiter is finished!"); }

        using SizeType = common::RawDelimSizeType;

        [[nodiscard]] auto get_data_view(std::size_t line_num) const -> OutputType
        {
            assert(line_number < get_line_num());
            return std::string_view{ output_data_[line_num].data(), output_data_[line_num].size() };
        }

        void run_task(const auto& prev_data_converter, std::size_t line_number)
        {
            assert(line_number < get_line_num());
            auto& output_data = output_data_[line_number];
            output_data.clear();
            convert(prev_data_converter.get_data_view(line_number), output_data);
        }

      private:
        std::vector<std::vector<char>> output_data_;
        static void convert(std::string_view input, std::vector<char>& output);
    };
} // namespace srs::process
