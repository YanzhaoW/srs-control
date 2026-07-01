#include "srs/Application.hpp"
#include "srs/ConfigHandler.hpp"
#include "srs/SRSWorld.hpp"
#include "srs/devices/Configuration.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include "srs/writers/DataWriterOptions.hpp"
#include <CLI/CLI.hpp>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <format>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

using std::exception;

using srs::common::internal::get_enum_dash_map;
using srs::common::internal::get_enum_dashed_name;
using srs::common::internal::get_enum_dashed_names;

constexpr auto SPDLOG_LOG_NAMES = get_enum_dashed_names<spdlog::level::level_enum>();
const auto spdlog_map = get_enum_dash_map<spdlog::level::level_enum>();

namespace
{
    auto get_file_size(std::string_view filename) -> std::size_t
    {
        namespace fs = std::filesystem;
        auto file_path = fs::path{ filename };
        if (fs::exists(file_path))
        {
            return std::filesystem::file_size(file_path);
        }
        spdlog::error("File with the name {:?} doesn't exist!", filename);
        return 0;
    }

} // namespace

constexpr auto DEFAULT_RUN_TIME_S = 5;
constexpr auto DEFAULT_DELAY_TIME_NS = 10000000;
// NOLINTNEXTLINE (bugprone-exception-escape)
auto main(int argc, char** argv) -> int
{
    auto cli_args = CLI::App{ "SRS integration test" };

    auto is_continuous_output = false;
    const auto input_filename = std::string{ "test_data.bin" };
    auto output_filenames = std::vector<std::string>{ "" };

    auto sent_n_frames = 0;
    auto received_n_frames = 0;
    auto n_data_ports = 1;
    try
    {
        auto delay_time = std::size_t{ DEFAULT_DELAY_TIME_NS };
        auto n_output_split = 1;
        auto n_instances = std::size_t{ 1 };
        auto run_time = DEFAULT_RUN_TIME_S;
        auto spdlog_level = spdlog::level::info;
        const auto home_dir = []() -> std::string
        {
            const auto* home_var = getenv("HOME");
            if (home_var != nullptr)
            {
                return std::string{ home_var };
            }
            return {};
        }();
        auto json_filepath = home_dir.empty() ? "" : std::format("{}/.config/srs-control/config.json", getenv("HOME"));
        argv = cli_args.ensure_utf8(argv);
        auto app_config = srs::Config{};

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
        cli_args.add_option("-s, --split-output", n_output_split, "Splitting the output data into different files.")
            ->capture_default_str();
        cli_args
            .add_option(
                "-d, --delay-time", delay_time, "Set the delay time (ns) when sending the data from the server.")
            ->capture_default_str();
        cli_args.add_option("-r, --run-time", run_time, "Set the run time (seconds) when --cont is enabled.")
            ->capture_default_str();
        cli_args.add_option("-n, --instances", n_instances, "Set the duplicated instances to send to the same port.")
            ->capture_default_str();
        cli_args.add_option("-c, --config-file", json_filepath, "Set the path of the JSON config file")
            ->capture_default_str();

        cli_args.parse(argc, argv);

        auto app = srs::App{};
        spdlog::set_level(spdlog_level);

        auto world = srs::test::World{ app,
                                       { .is_continue = is_continuous_output,
                                         .delay_time = delay_time,
                                         .n_instances = n_instances,
                                         .input_filename = "test_data.bin" } };

        auto is_ok = srs::config::set_config_from_json(app_config, json_filepath);

        if (not is_ok)
        {
            throw std::runtime_error("Error occurred when reading configuration files.");
        }
        n_data_ports = app_config.fec_data_receive_ports.size();

        app.set_options(std::move(app_config));
        // auto output_file = std::vector<std::string>{ "test_output.bin" };
        app.set_output_filenames(output_filenames, n_output_split);
        world.init();

        app.start_workflow();

        auto switch_off_thread = std::jthread{};
        app.read_data();

        world.launch_server();
        app.switch_on();
        switch_off_thread = std::jthread(
            [&app, &world, is_continuous_output, &run_time, &sent_n_frames, &received_n_frames]()
            {
                if (is_continuous_output)
                {
                    std::this_thread::sleep_for(std::chrono::seconds{ run_time });
                }
                else
                {
                    world.for_each_server([](auto& server) { server->wait_for_data_sender(); });
                }
                app.exit_and_switch_off();
                sent_n_frames += world.get_n_frames_sent();
                received_n_frames += app.get_workflow_handler().get_frame_counts();
            });

        app.wait_for_workflow();
        spdlog::info("Waiting for srs server to stop.");
        world.join();
        spdlog::info("Reaching the end of the main block.");
    }
    catch (const CLI::ParseError& e)
    {
        cli_args.exit(e);
        return 0;
    }
    catch (const exception& ex)
    {
        spdlog::critical("exception occurred: {}", ex.what());
        return EXIT_FAILURE;
    }

    if (sent_n_frames != received_n_frames)
    {
        spdlog::error("{} frames sent but {} frames received! Drop rate: {}",
                      sent_n_frames,
                      received_n_frames,
                      1. - static_cast<double>(received_n_frames) / static_cast<double>(sent_n_frames));
        return EXIT_FAILURE;
    }
    spdlog::info("{} frames are correctly received!", sent_n_frames);

    auto wrong_file_size =
        not is_continuous_output and
        not std::ranges::all_of(
            output_filenames,
            [&input_filename, n_data_ports](std::string_view name) -> bool
            {
                using enum srs::DataWriterOption;
                auto file_type = std::get<0>(srs::get_filetype_from_filename(name));
                if (file_type == no_output or file_type == udp)
                {
                    return true;
                }
                if (name.ends_with(".bin"))
                {
                    const auto output_size = get_file_size(name);
                    const auto input_size = n_data_ports * get_file_size(input_filename);
                    if (output_size == input_size)
                    {
                        return true;
                    }
                    spdlog::error("Output binary files don't have correct sizes! Expected size: {}B, but got {}B.",
                                  input_size,
                                  output_size);
                    return false;
                }
                if (get_file_size(name) != 0)
                {
                    return true;
                }
                spdlog::error("Output files are empty!");
                return false;
            });
    if (wrong_file_size)
    {
        return EXIT_FAILURE;
    }
    spdlog::info("All output files have correct sizes!");

    return 0;
}
