#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/utils/CommonConcepts.hpp"
#include "srs/writers/DataWriterOptions.hpp"
#include <cassert>
#include <cstddef>
#include <fmt/format.h>
#include <fstream>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>

namespace srs::writer
{
    class BinaryFile : public process::WriterTask<DataWriterOption::bin, std::string_view, std::size_t>
    {
      public:
        static constexpr auto IsStructType = false;

        BinaryFile(const std::string& filename, process::DataConvertOptions convert_mode, std::size_t n_lines);
        BinaryFile(const BinaryFile&) = delete;
        BinaryFile(BinaryFile&&) = delete;
        BinaryFile& operator=(const BinaryFile&) = delete;
        BinaryFile& operator=(BinaryFile&&) = delete;
        ~BinaryFile();

        auto operator()(const OutputTo<InputType> auto& prev_data_converter, std::size_t line_number) -> OutputType
        {
            assert(line_number < get_n_lines());
            auto input_data = prev_data_converter.get_data_view(line_number);
            output_data_[line_number] += input_data.size();
            output_streams_[line_number] << input_data;
            return output_data_[line_number];
        }
        void close();

        [[nodiscard]] auto get_filename() const -> const std::string& { return file_name_; }
        [[nodiscard]] auto get_data(std::size_t line_number) const -> OutputType { return output_data_[line_number]; }

      private:
        std::string file_name_;
        std::vector<OutputType> output_data_;
        std::vector<std::ofstream> output_streams_;
    };

} // namespace srs::writer
