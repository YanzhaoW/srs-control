#include <spdlog/spdlog.h>
#include <srs/readers/RawFrameReader.hpp>
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

    auto RawFrame::read_one_frame(std::vector<char>& binary_data, std::ifstream& input_file) -> std::size_t
    {
        if (not input_file.is_open())
        {
            spdlog::critical("The input file is not open!");
            return 0;
        }
        binary_data.reserve(common::LARGE_READ_MSG_BUFFER_SIZE);
        std::array<char, sizeof(common::RawDelimSizeType)> size_buffer{};
        auto size = common::RawDelimSizeType{};
        if (input_file.eof())
        {
            spdlog::info("End of the binary file.");
            return 0;
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

    auto RawFrame::read_one_frame() -> std::string_view
    {
        read_one_frame(input_buffer_, input_file_);
        return std::string_view{ input_buffer_.data(), input_buffer_.size() };
    }
} // namespace srs::reader
