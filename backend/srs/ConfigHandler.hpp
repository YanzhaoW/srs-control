#pragma once

#include <glaze/glaze.hpp>
#include <spdlog/spdlog.h>
#include <srs/devices/Configuration.hpp>

namespace srs::config
{
    inline void output_config_to_json(const Config& app_config, std::string_view json_filename)
    {
        if (std::filesystem::exists(json_filename))
        {
            spdlog::critical("Cannot dump the default value to the configuration file {:?} as it's already existed!",
                             json_filename);
            return;
        }
        const auto parent_dir = std::filesystem::absolute(std::filesystem::path{ json_filename }).parent_path();
        std::filesystem::create_directories(parent_dir);
        auto error_code = glz::write_file_json<glz::opts{ .prettify = true, .new_lines_in_arrays = false }>(
            app_config, json_filename, std::string{});
    }

    inline void set_config_from_json(Config& app_config, std::string_view json_filename)
    {
        if (json_filename.empty())
        {
            return;
        }
        if (not std::filesystem::exists(json_filename))
        {
            spdlog::warn("configuration file {:?} doesn't exist. Creating a new file with default values",
                         json_filename);
            output_config_to_json(app_config, json_filename);
        }
        else
        {
            auto error_code = glz::read_file_json(app_config, json_filename, std::string{});
        }
    }

} // namespace srs::config
