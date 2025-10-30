#include "srs/SRSWorld.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <CLI/CLI.hpp>
#include <chrono>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <gtest/gtest.h>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <string>
#include <thread>
#include <vector>

using std::exception;

using srs::common::internal::get_enum_dash_map;
using srs::common::internal::get_enum_dashed_name;
using srs::common::internal::get_enum_dashed_names;

constexpr auto SPDLOG_LOG_NAMES = get_enum_dashed_names<spdlog::level::level_enum>();
const auto spdlog_map = get_enum_dash_map<spdlog::level::level_enum>();

auto main(int argc, char** argv) -> int
{
    auto cli_args = CLI::App{ "SRS integration test" };

    try
    {
        argv = cli_args.ensure_utf8(argv);

        auto is_continuous_output = false;
        auto spdlog_level = spdlog::level::info;
        auto sleep_time = 5;
        auto output_filenames = std::vector<std::string>{ "" };

        cli_args
            .add_option("-l, --log-level",
                        spdlog_level,
                        fmt::format("Set log level\nAvailable options: [{}]", fmt::join(SPDLOG_LOG_NAMES, ", ")))
            ->transform(CLI::CheckedTransformer(spdlog_map, CLI::ignore_case).description(""))
            ->default_str(get_enum_dashed_name(spdlog_level));
        cli_args.add_flag("--cont", is_continuous_output, "Set the emulator continuous output");
        cli_args.add_option("-o, --output-files", output_filenames, "Set output file (or socket) names")
            ->capture_default_str()
            ->expected(0, -1);

        cli_args.parse(argc, argv);

        spdlog::set_level(spdlog_level);
        auto world = srs::test::World{ "test_data.bin" };
        world.set_continue_output(is_continuous_output);

        auto& app = world.get_app();

        // auto output_file = std::vector<std::string>{ "test_output.bin" };
        app.set_output_filenames(output_filenames);

        world.init();
        world.launch_server();

        app.read_data();
        app.switch_on();

        auto switch_off_thread = std::jthread(
            [&app, &world, is_continuous_output, &sleep_time]()
            {
                if (is_continuous_output)
                {
                    std::this_thread::sleep_for(std::chrono::seconds{ sleep_time });
                }
                else
                {
                    world.get_server()->wait_for_data_sender();
                }
                app.exit_and_switch_off();
            });

        app.start_workflow();
    }
    catch (const CLI::ParseError& e)
    {
        cli_args.exit(e);
    }
    catch (const exception& ex)
    {
        spdlog::critical("exception occurred: {}", ex.what());
    }
    return 0;
}
