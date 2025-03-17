#pragma once

#include <filesystem>

#include <srs/converters/DataConvertOptions.hpp>

namespace srs
{
    enum class DataWriterOption : uint8_t
    {

        no_output = 0x00,
        root = 0x01,
        json = 0x02,
        bin = 0x04,
        udp = 0x10,
    };

    inline auto get_filetype_from_filename(std::string_view filename)
        -> std::tuple<DataWriterOption, process::DataConvertOptions>
    {
        using enum DataWriterOption;
        using enum process::DataConvertOptions;
        namespace fs = std::filesystem;

        if (auto pos = filename.find(':'); pos != std::string::npos)
        {
            return { udp, proto };
        }

        const auto file_ext = fs::path{ filename }.extension().string();
        if (file_ext == ".bin" or file_ext == ".lmd")
        {
            return { bin, raw_frame };
        }
        if (file_ext == ".binpb")
        {
            return { bin, proto_frame };
        }
        if (file_ext == ".root")
        {
            return { root, structure };
        }
        if (file_ext == ".json")
        {
            return { json, structure };
        }

        return { no_output, none };
    }

} // namespace srs
