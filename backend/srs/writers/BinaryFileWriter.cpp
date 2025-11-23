#include "BinaryFileWriter.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <cassert>
#include <cstddef>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <ios>
#include <ranges>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>

namespace srs::writer
{
    BinaryFile::BinaryFile(const std::string& filename, process::DataConvertOptions convert_mode, std::size_t n_lines)
        : WriterTask{ "BinaryWriter", convert_mode, n_lines }
        , file_name_{ filename }
    {
        assert(n_lines > 0);
        output_data_.resize(n_lines);
        output_streams_.reserve(n_lines);
        for (auto idx : std::views::iota(0, static_cast<int>(n_lines)))
        {
            auto full_filename = (n_lines == 1) ? filename : common::insert_index_to_filename(filename, idx);
            auto& ofstream = output_streams_.emplace_back(full_filename, std::ios::trunc);
            if (not ofstream.is_open())
            {
                throw std::runtime_error(fmt::format("Filename {:?} cannot be open!", filename));
            }
        }
    }

    BinaryFile::~BinaryFile()
    {
        close();
        spdlog::debug(
            "Writer: Binary file {} data size written: \n\t{}",
            file_name_,
            fmt::join(
                std::views::enumerate(output_data_) |
                    std::views::transform(
                        [](const auto& idex_data_size) -> std::string
                        { return fmt::format("{}: {}", std::get<0>(idex_data_size), std::get<1>(idex_data_size)); }),
                "\n\t"));
        spdlog::info("Writer: Binary file writer with the base name {:?} is closed successfully.", file_name_);
    }
    void BinaryFile::close()
    {
        for (auto& file_stream : output_streams_)
        {
            file_stream.close();
        }
    }
} // namespace srs::writer
