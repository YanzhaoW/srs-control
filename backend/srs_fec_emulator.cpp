#include "srs/ConfigHandler.hpp"
#include "srs/emulators/World.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <CLI/CLI.hpp>
#include <cstdlib>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string>

using srs::common::internal::get_enum_dash_map;
using srs::common::internal::get_enum_dashed_name;
using srs::common::internal::get_enum_dashed_names;

constexpr auto SPDLOG_LOG_NAMES = get_enum_dashed_names<spdlog::level::level_enum>();
const auto spdlog_map = get_enum_dash_map<spdlog::level::level_enum>();

auto main(int argc, char** argv) -> int
{

    auto spdlog_level = spdlog::level::info;
    auto cli_args = CLI::App{ "SRS FEC emulators" };
    argv = cli_args.ensure_utf8(argv);

    auto is_dump_needed = false;

    auto config_filepath = std::string{};

    auto dump_config_callback = [&config_filepath, &is_dump_needed](const std::string& filename)
    {
        is_dump_needed = true;
        config_filepath = filename;
    };

    auto config = srs::emulator::World::Config{};

    cli_args
        .add_option("-l, --log-level",
                    spdlog_level,
                    fmt::format("Set log level\nAvailable options: [{}]", fmt::join(SPDLOG_LOG_NAMES, ", ")))

        ->transform(CLI::CheckedTransformer(spdlog_map, CLI::ignore_case).description(""))
        ->default_str(get_enum_dashed_name(spdlog_level));
    ;
    auto dump_option = cli_args
                           .add_option_function<std::string>(
                               "--dump-config", dump_config_callback, "Dump default configuration to the file")
                           ->default_val("fec_emulator.yaml")
                           ->run_callback_for_default()
                           ->expected(0, 1);

    auto config_option = cli_args.add_option("config_file", config_filepath, "Set the path of the config file")
                             ->capture_default_str()
                             ->excludes(dump_option);

    CLI11_PARSE(cli_args, argc, argv);

    spdlog::set_level(spdlog_level);

    if (is_dump_needed)
    {
        auto is_not_ok = srs::config::output_config_to_file(config, config_filepath);
        if (is_not_ok)
        {
            spdlog::error("{}", is_not_ok.value());
            return EXIT_FAILURE;
        }
        else
        {
            spdlog::info("Config file {:?} is created.", config_filepath);
        }
        return EXIT_SUCCESS;
    }

    auto is_not_ok = srs::config::set_config_from_file(config, config_filepath);

    auto world = srs::emulator::World{ config };

    world.init();

    world.launch();

    return EXIT_SUCCESS;
}
