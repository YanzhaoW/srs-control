#include "CLIOptionsMap.hpp"
#include <CLI/CLI.hpp>
#include <print>
#include <spdlog/spdlog.h>
#include <srs/Application.hpp>

#ifdef HAS_ROOT
#include <TROOT.h>
#endif

auto main(int argc, char** argv) -> int
{
    auto cli_args = CLI::App{ "SRS system command line interface" };
    try
    {
        argv = cli_args.ensure_utf8(argv);

        auto spdlog_level = spdlog::level::info;
        auto print_mode = srs::common::DataPrintMode::print_speed;
        auto output_filenames = std::vector<std::string>{ "output.bin" };
        auto is_version_print = false;
        auto is_root_version_print = false;

        cli_args.add_flag("-v, --version", is_version_print, "show the current version");
        cli_args.add_flag("--root-version", is_root_version_print, "show the ROOT version if used");
        cli_args.add_option("-l, --log-level", spdlog_level, "set log level")
            ->transform(CLI::CheckedTransformer(spd_log_map, CLI::ignore_case))
            ->capture_default_str();
        cli_args.add_option("-p, --print-mode", print_mode, "set data print mode")
            ->transform(CLI::CheckedTransformer(print_mode_map, CLI::ignore_case))
            ->capture_default_str();
        cli_args.add_option("-o, --output-files", output_filenames, "set output file (or socket) names")
            ->capture_default_str()
            ->expected(0, -1);
        cli_args.parse(argc, argv);

        if (is_version_print)
        {
            std::println("{}", SRS_PROJECT_VERSION);
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
        app.set_remote_endpoint(srs::common::DEFAULT_SRS_IP, srs::common::DEFAULT_SRS_CONTROL_PORT);
        app.set_print_mode(print_mode);
        app.set_output_filenames(output_filenames);

        app.init();
        app.read_data();
        app.switch_on();
        app.start_workflow();
        // app.wait_for_finish();
    }
    catch (const CLI::ParseError& e)
    {
        cli_args.exit(e);
    }
    catch (std::exception& ex)
    {
        spdlog::critical("Exception occured: {}", ex.what());
    }

    return 0;
}
