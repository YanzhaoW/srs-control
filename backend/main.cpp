#include "srs/Application.hpp"
#include "srs/ConfigHandler.hpp"
#include "srs/devices/Configuration.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <CLI/CLI.hpp>
#include <cstdlib>
#include <exception>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <format>
#include <magic_enum/magic_enum.hpp>
#include <print>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef HAS_ROOT
#include <TROOT.h>
#endif

using srs::common::internal::get_enum_dash_map;
using srs::common::internal::get_enum_dashed_name;
using srs::common::internal::get_enum_dashed_names;

constexpr auto ACTION_MODE_NAMES = get_enum_dashed_names<srs::common::ActionMode>();
constexpr auto SPDLOG_LOG_NAMES = get_enum_dashed_names<spdlog::level::level_enum>();
constexpr auto PRINT_MODE_NAMES = get_enum_dashed_names<srs::common::DataPrintMode>();

const auto print_mode_map = get_enum_dash_map<srs::common::DataPrintMode>();
const auto spdlog_map = get_enum_dash_map<spdlog::level::level_enum>();
const auto action_mode_map = get_enum_dash_map<srs::common::ActionMode>();

auto main(int argc, char** argv) -> int
{
    auto cli_args = CLI::App{ "SRS system command line interface" };
    try
    {
        argv = cli_args.ensure_utf8(argv);

        auto app_config = srs::Config{};

        auto spdlog_level = spdlog::level::info;
        auto print_mode = srs::common::DataPrintMode::print_speed;
        auto output_filenames = std::vector<std::string>{ "" };
        auto is_version_print = false;
        auto is_root_version_print = false;
        auto is_dump_needed = false;
        const auto home_dir = std::string_view{ getenv("HOME") };
        auto json_filepath = home_dir.empty() ? "" : std::format("{}/.config/srs-control/config.json", getenv("HOME"));
        auto action_mode_enum = srs::common::ActionMode::all;
        auto dump_config_callback = [&json_filepath, &is_dump_needed](const std::string& filename)
        {
            is_dump_needed = true;
            json_filepath = filename;
        };
        cli_args.add_flag("-v, --version", is_version_print, "Show the current version");
        cli_args.add_flag("--root-version", is_root_version_print, "Show the ROOT version if used");

        cli_args
            .add_option(
                "-a, --action",
                action_mode_enum,
                fmt::format("Set the action of program\nAvailable options: [{}]", fmt::join(ACTION_MODE_NAMES, ", ")))
            ->transform(CLI::CheckedTransformer(action_mode_map, CLI::ignore_case).description(""))
            ->default_str(get_enum_dashed_name(action_mode_enum));

        cli_args
            .add_option("-l, --log-level",
                        spdlog_level,
                        fmt::format("Set log level\nAvailable options: [{}]", fmt::join(SPDLOG_LOG_NAMES, ", ")))
            ->transform(CLI::CheckedTransformer(spdlog_map, CLI::ignore_case).description(""))
            ->default_str(get_enum_dashed_name(spdlog_level));

        cli_args
            .add_option("-p, --print-mode",
                        print_mode,
                        fmt::format("Set data print mode\nAvailable options: [{}]", fmt::join(PRINT_MODE_NAMES, ", ")))
            ->transform(CLI::CheckedTransformer(print_mode_map, CLI::ignore_case).description(""))
            ->default_str(get_enum_dashed_name(print_mode));

        cli_args.add_option("-c, --config-file", json_filepath, "Set the path of the JSON config file")
            ->capture_default_str();
        cli_args
            .add_option_function<std::string>(
                "--dump-config", dump_config_callback, "Dump default configuration to the file")
            ->default_val(json_filepath)
            ->run_callback_for_default()
            ->expected(0, 1);

        cli_args.add_option("-o, --output-files", output_filenames, "Set output file (or socket) names")
            ->capture_default_str()
            ->expected(0, -1);
        cli_args.parse(argc, argv);

        if (is_version_print)
        {
            std::println("{}", SRS_PROJECT_VERSION);
            return 0;
        }

        if (is_dump_needed)
        {
            srs::config::output_config_to_json(app_config, json_filepath);
            return 0;
        }

        if (is_root_version_print)
        {
#ifdef HAS_ROOT
            std::println("{}", gROOT->GetVersion());
#else
            std::println("ROOT is not built");
#endif
            return 0;
        }

        spdlog::set_level(spdlog_level);

        auto app = srs::App{};

        srs::config::set_config_from_json(app_config, json_filepath);
        app.set_options(std::move(app_config));
        app.set_print_mode(print_mode);
        app.set_output_filenames(output_filenames);

        app.init();

        switch (action_mode_enum)
        {
            using enum srs::common::ActionMode;
            case all:
            {
                app.read_data();
                app.switch_on();
                app.start_workflow();
                break;
            }
            case acq_on:
            {
                app.switch_on();
                break;
            }
            case acq_off:
            {
                app.switch_off();
                break;
            }
            case read_only:
            {

                app.read_data();
                app.start_workflow();
                break;
            }
            default:
                return 0;
        }

        // app.wait_for_finish();
    }
    catch (const CLI::ParseError& e)
    {
        cli_args.exit(e);
    }
    catch (std::exception& ex)
    {
        spdlog::critical("Exception occurred: {}", ex.what());
    }

    return 0;
}
