#pragma once

#include "srs/utils/GlazeCustom.hpp" // IWYU pragma: keep
#include <filesystem>
#include <fmt/format.h>
#include <glaze/core/opts.hpp>
#include <glaze/core/reflect.hpp>
#include <glaze/glaze.hpp>
#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>
#include <glaze/yaml.hpp>
#include <glaze/yaml/opts.hpp>
#include <glaze/yaml/read.hpp>
#include <glaze/yaml/write.hpp>
#include <optional>
#include <spdlog/spdlog.h>
#include <srs/devices/Configuration.hpp>
#include <string>
#include <string_view>

namespace srs::config
{

    enum class ConfigFileType
    {
        json,
        yaml,
        invalid,
    };

    inline auto resolve_filetype(std::string_view filename) -> ConfigFileType
    {
        using enum ConfigFileType;
        if (filename.ends_with(".json"))
        {
            return json;
        }
        else if (filename.ends_with(".yml") or filename.ends_with(".yaml"))
        {

            return yaml;
        }
        return invalid;
    }

    inline auto output_config_to_json(const Config& app_config, std::string_view json_filename)
        -> std::optional<std::string>
    {
        auto buffer = std::string{};
        auto error_code = glz::write_file_json<glz::opts{ .prettify = true }>(app_config, json_filename, buffer);
        if (error_code)
        {
            return fmt::format("Error occurred during writing the json file {}: {}",
                               json_filename,
                               glz::format_error(error_code, buffer));
        }
        return {};
    }

    inline auto set_config_from_json(Config& app_config, std::string_view json_filename) -> std::optional<std::string>
    {
        auto buffer = std::string{};
        auto error_code = glz::read_file_json(app_config, json_filename, buffer);
        if (error_code)
        {
            return fmt::format("Error occurred during reading the json file {}: {}",
                               json_filename,
                               glz::format_error(error_code, buffer));
        }
        return {};
    }

    inline auto output_config_to_yaml(const Config& app_config, std::string_view filename) -> std::optional<std::string>

    {
        auto error_code = glz::write_file_yaml<glz::yaml::yaml_opts{}>(app_config, filename);
        if (error_code)
        {
            return fmt::format(
                "Error occurred during writing the yaml file {}: {}", filename, glz::format_error(error_code));
        }
        return {};
    }

    inline auto set_config_from_yaml(Config& app_config, std::string_view filename) -> std::optional<std::string>
    {
        auto error_code = glz::read_file_yaml(app_config, filename);
        if (error_code)
        {
            return fmt::format(
                "Error occurred during reading the json file {}: {}", filename, glz::format_error(error_code));
        }
        return {};
    }

    inline auto output_config_to_file(const Config& app_config, std::string_view filename, bool is_overwrite = false)
        -> std::optional<std::string>
    {
        if (not is_overwrite and std::filesystem::exists(filename))
        {
            return fmt::format("Cannot dump the default value to the configuration file {:?} as it's already existed!",
                               filename);
        }

        const auto parent_dir = std::filesystem::absolute(std::filesystem::path{ filename }).parent_path();
        std::filesystem::create_directories(parent_dir);

        auto file_type = resolve_filetype(filename);
        using enum ConfigFileType;
        switch (file_type)
        {
            case yaml:
                return output_config_to_yaml(app_config, filename);
            case json:
                return output_config_to_json(app_config, filename);
            case invalid:
            default:
                return fmt::format("Filename {:?} must have a suffix of .yml, .yaml or .json", filename);
        }
        return {};
    }

    inline auto set_config_from_file(Config& app_config, std::string_view filename, bool is_default_generated = true)
        -> std::optional<std::string>
    {
        if (filename.empty())
        {
            return std::string{ "Config filename is empty!" };
        }
        auto file_type = resolve_filetype(filename);
        if (not std::filesystem::exists(filename))
        {
            if (is_default_generated)
            {
                spdlog::warn("configuration file {:?} doesn't exist. Creating a new file with default values",
                             filename);
                output_config_to_file(app_config, filename);
            }
            else
            {
                return fmt::format("configuration file {:?} doesn't exist!", filename);
            }
        }
        else
        {
            using enum ConfigFileType;
            switch (file_type)
            {
                case yaml:
                    return set_config_from_yaml(app_config, filename);
                    break;
                case json:
                    return set_config_from_json(app_config, filename);
                    break;
                case invalid:
                default:
                    return fmt::format("Filename {:?} must have a suffix of .yml, .yaml or .json", filename);
            }
        }
        return {};
    }
} // namespace srs::config
