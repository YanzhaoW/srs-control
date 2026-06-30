#include "srs/utils/CommonDefinitions.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <CLI/CLI.hpp>
#include <cassert>
#include <cstdlib>
#include <exception>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <print>
#include <ranges>
#include <re2/re2.h>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace srs
{
    class LogFile
    {
      public:
        LogFile(const std::string& filename)
            : input_file_{ filename, std::ios::in }
        {
            if (not input_file_.is_open())
            {
                throw std::runtime_error(std::format("failed to open the file {}", filename));
            }
            grow_to(default_init_n_lines);
        }

        void read()
        {
            while (true)
            {
                if (line_counter_ >= content_.size())
                {
                    grow_to(content_.size() * 2);
                }
                content_[line_counter_].clear();
                if (not std::getline(input_file_, content_[line_counter_]))
                {
                    break;
                }
                if (content_[line_counter_].ends_with(common::file_logger_new_instance_str))
                {
                    line_counter_ = 0;
                }
                else
                {
                    ++line_counter_;
                }
            }
        }

        void print()
        {
            auto filter_view =
                call_map_ | std::views::filter([](const auto& pair) -> bool { return pair.second != 0; });

            if (std::ranges::distance(filter_view.begin(), filter_view.end()) == 0)
            {
                std::println("All recorded scopes have exited correctly!");
                return;
            }

            std::println("Following recorded scopes didn't exit:");
            for (const auto& [key, value] : filter_view)
            {
                std::println("\t{}", key);
            }
        }

        void print_last_log()
        {
            for (const auto& line_str : content_ | std::views::take(line_counter_))
            {
                std::println("{}", line_str);
            }
        }

        void analysis()
        {
            auto key_str = std::string{};
            auto begin_re = RE2{ std::format("\\[.*\\]\\[\\w+\\]\\[\\d+\\] {} (.*)", srs::common::exit_logger_begin) };
            auto end_re = RE2{ std::format("\\[.*\\]\\[\\w+\\]\\[\\d+\\] {} (.*)", srs::common::exit_logger_end) };
            for (const auto& line_str : content_ | std::views::take(line_counter_))
            {
                if (RE2::FullMatch(line_str, begin_re, &key_str))
                {
                    auto entry = call_map_.try_emplace(key_str, 0).first;
                    ++(entry->second);
                    key_str.clear();
                }
                else if (RE2::FullMatch(line_str, end_re, &key_str))
                {
                    auto entry = call_map_.try_emplace(key_str, 0).first;
                    --(entry->second);
                    key_str.clear();
                }
            }
        }

      private:
        static const auto default_init_n_lines = 1000;
        static const auto default_init_n_chars_per_line = 300;
        std::ifstream input_file_;
        std::size_t line_counter_ = 0;
        std::vector<std::string> content_;
        std::unordered_map<std::string, int> call_map_;

        void grow_to(std::size_t n_lines)
        {
            if (n_lines <= content_.size())
            {
                return;
            }
            const auto old_size = content_.size();
            content_.resize(n_lines);
            for (auto& str : content_ | std::views::drop(old_size))
            {
                str.reserve(default_init_n_chars_per_line);
            }
        }
    };
} // namespace srs

// NOLINTNEXTLINE (bugprone-exception-escape)
auto main(int argc, char** argv) -> int
{

    auto cli_args = CLI::App{ "Logger checking of srs_control program." };

    auto default_log_filename = srs::common::get_default_log_path().string();

    auto is_show_last_log = false;

    try
    {
        cli_args.add_option("-f, --file", default_log_filename, "Set the path to the log file")
            ->capture_default_str()
            ->expected(0, 1);
        cli_args.add_flag("-l, --show-last-log", is_show_last_log, "Show last log content");

        cli_args.parse(argc, argv);

        auto log_file = srs::LogFile(default_log_filename);

        log_file.read();

        if (is_show_last_log)
        {
            log_file.print_last_log();
            return EXIT_SUCCESS;
        }

        log_file.analysis();
        log_file.print();
    }
    catch (const CLI::ParseError& e)
    {
        cli_args.exit(e);
    }
    catch (std::exception& ex)
    {
        std::println(stderr, "Exception occurred: {}", ex.what());
    }
    catch (...)
    {
        std::println(stderr, "A unrecognized exception occurred!");
    }

    return EXIT_SUCCESS;
}
