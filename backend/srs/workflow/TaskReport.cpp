#include "TaskReport.hpp"
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace srs::workflow
{
    namespace
    {
        auto print_stat(const TaskReport::Stat& stat, std::string_view task_name, auto line_number) -> std::string
        {
            return (stat.total_time == 0) ? std::string{}
                                          : fmt::format("\t|{:^30}|{:^4}|{:^30}|{:^30.1f}|",
                                                        task_name,
                                                        line_number,
                                                        stat.total_time,
                                                        static_cast<double>(stat.total_time) /
                                                            static_cast<double>(stat.total_sample_size));
        }
    } // namespace

    TaskReport::~TaskReport()
    {
        spdlog::debug(
            "Performance report from tasks:\n"
            "\t|{:^30}|{:^4}|{:^30}|{:^30}|\n"
            "{}",
            "Task name",
            "n",
            "Total time (ns)",
            "Average time (ns)",
            records_ |
                std::views::transform(
                    [](const auto& key_val) -> std::string
                    {
                        return std::views::transform(
                                   std::views::zip(std::views::iota(0), key_val.second),
                                   [&task_name = key_val.first](auto idx_stat)
                                   { return print_stat(std::get<1>(idx_stat), task_name, std::get<0>(idx_stat)); }) |
                               std::views::filter([](const auto& str) -> bool { return not str.empty(); }) |
                               std::views::join_with('\n') | std::ranges::to<std::string>();
                    }) |
                std::views::filter([](const auto& str) -> bool { return not str.empty(); }) |
                std::views::join_with('\n') | std::ranges::to<std::string>());
    }
} // namespace srs::workflow
