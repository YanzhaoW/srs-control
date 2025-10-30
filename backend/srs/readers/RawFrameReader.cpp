#include "srs/utils/CommonDefinitions.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fstream>
#include <ios>
#include <spdlog/spdlog.h>
#include <srs/readers/RawFrameReader.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <zpp_bits.h>

namespace srs::reader
{

    RawFrame::RawFrame(const std::string& filename)
        : input_filename_{ filename }
        , input_file_{ filename, std::ios::binary }
    {
        if (input_file_.fail())
        {
            throw std::runtime_error{ fmt::format("Cannot open the file {:?}", input_filename_) };
        }
        spdlog::debug("Open the binary file {:?}", input_filename_);
    }

    auto RawFrame::read_one_frame(std::vector<char>& binary_data, std::ifstream& input_file)
        -> std::expected<std::size_t, std::string>
    {
        if (not input_file.is_open())
        {
            return std::unexpected(fmt::format("The input file {} is not open!", input_filename_));
        }
        binary_data.reserve(common::LARGE_READ_MSG_BUFFER_SIZE);
        std::array<char, sizeof(common::RawDelimSizeType)> size_buffer{};
        auto size = common::RawDelimSizeType{};
        if (input_file.eof())
        {
            return 0;
            // return std::unexpected("End of the binary file.");
        }

        input_file.read(size_buffer.data(), static_cast<int64_t>(size_buffer.size()));
        auto serialize_to = zpp::bits::in{ size_buffer, zpp::bits::endian::big{} };
        serialize_to(size).or_throw();
        binary_data.resize(size, 0);
        auto read_size = static_cast<std::size_t>(input_file.read(binary_data.data(), size).gcount());

        if (read_size != size)
        {
            spdlog::warn("Binary Data is not extracted correctly from the input file");
        }

        return read_size;
    }

    void RawFrame::reset()
    {
        input_file_.clear();
        input_file_.seekg(0, std::ios::beg);
    }

    auto RawFrame::read_one_frame() -> std::expected<std::string_view, std::string>
    {
        auto res = read_one_frame(input_buffer_, input_file_);
        if (res.has_value())
        {
            return std::string_view{ input_buffer_.data(), input_buffer_.size() };
        }
        return std::unexpected{ res.error() };
    }
} // namespace srs::reader
