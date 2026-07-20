#include "AppReport.hpp"
#include "srs/utils/CommonConcepts.hpp"
#include <algorithm>
#include <cstddef>
#include <format>
#include <limits>
#include <ranges>
#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
#include <tabulate/color.hpp>
#include <tabulate/font_align.hpp>
#include <tabulate/font_style.hpp>
#include <tabulate/table.hpp>
#include <tabulate/termcolor.hpp>
#include <type_traits>
#include <vector>

namespace srs
{
    namespace
    {
        using Row = tabulate::Table::Row_t;

        void format_solid_border(tabulate::Table& table)
        {
            table.format().border_top("─").border_bottom("─").border_left("│").border_right("│");
        }

        auto format_records(const auto& records, const Row& title_names, auto row_adder) -> std::string
        {
            using namespace tabulate;

            auto content_table = tabulate::Table{};
            format_solid_border(content_table);

            content_table.add_row(title_names);
            for (auto& cell : content_table.row(0))
            {
                cell.format().font_color(Color::yellow).font_align(FontAlign::center).font_style({ FontStyle::bold });
            }

            using native_type = std::remove_cvref_t<decltype(records)>;
            if constexpr (is_map_type<native_type>::value or is_vector_type<native_type>::value)
            {
                if (records.empty())
                {
                    return std::string{};
                }
                for (const auto& [name, split_record] : records)
                {
                    if constexpr (is_vector_type<std::remove_cvref_t<decltype(split_record)>>::value)
                    {
                        for (const auto [n_pipe, record] : std::views::zip(std::views::iota(0), split_record))
                        {
                            if (n_pipe != 0)
                            {
                                auto row = Row{ "" };
                                row_adder(row, n_pipe, record);
                                content_table.add_row(row);
                                content_table.row(content_table.size() - 1).format().border_top("-");
                                content_table[content_table.size() - 1][0].format().border_top(" ");
                            }
                            else
                            {

                                auto row = Row{ name };
                                row_adder(row, n_pipe, record);
                                content_table.add_row(row);
                            }
                        }
                    }
                    else
                    {
                        auto row = Row{ name };
                        row_adder(row, split_record);
                        content_table.add_row(row);
                    }
                }
            }
            else
            {
                auto row = Row{ records.first };
                row_adder(row, records.second);
                content_table.add_row(row);
            }

            content_table.column(0).format().font_align(FontAlign::left);

            for (const auto [idx, _] :
                 std::views::zip(std::views::iota(std::size_t{}), title_names) | std::views::drop(1))
            {
                content_table.column(idx).format().font_align(FontAlign::center);
            }

            auto sstream = std::stringstream{};
            sstream << termcolor::colorize << content_table;
            return sstream.str();
        }
    } // namespace

    void AppReport::report_sink_file_result()
    {
        auto str = format_records(sink_records_,
                                  {
                                      "Sink name",
                                      "Split",
                                      "Bytes written (KB)",
                                  },
                                  [](Row& row, int idx, std::size_t bytes)
                                  {
                                      const auto kb_written = bytes == 0 ? std::numeric_limits<double>::quiet_NaN()
                                                                         : static_cast<double>(bytes) / 1000.;
                                      row.emplace_back(std::format("{}", idx));
                                      row.emplace_back(std::format("{:.1f}", kb_written));
                                  });
        spdlog::debug("Bytes written to output sinks:\n{}", str);
    }

    void AppReport::report_task_result()
    {
        auto str = format_records(task_records_,
                                  {
                                      "Task name",
                                      "Split",
                                      "Total time (ms)",
                                      "Average time (ns)",
                                      "Frames read",
                                  },
                                  [](Row& row, int idx, const TaskStat& stat)
                                  {
                                      const auto total_time_ms = stat.total_time_ms;
                                      const auto ave_time =
                                          stat.total_time_ms == 0
                                              ? 0.
                                              : 1e6 * stat.total_time_ms / static_cast<double>(stat.total_sample_size);
                                      row.push_back(std::format("{}", idx));
                                      row.push_back(std::format("{:.1f}", total_time_ms));
                                      row.push_back(std::format("{:.1f}", ave_time));
                                      row.push_back(std::format("{}", stat.total_sample_size));
                                  });

        spdlog::debug("Performance report from required tasks:\n{}", str);
    }

    void AppReport::report_socket_result()
    {
        for (auto& [socket_name, stats] : switch_socket_records_)
        {
            std::ranges::sort(stats,
                              [](const auto& left, const auto& right) -> bool
                              { return left.second.send_time < right.second.send_time; });
            if (stats.empty())
            {
                continue;
            }
            const auto first_time = stats.front().second.send_time;
            for (auto& [_, stat] : stats)
            {
                stat.send_time_offset = stat.send_time - first_time;
            }
        }

        auto str = format_records(switch_socket_records_,
                                  {
                                      "Local socket",
                                      "Remote socket",
                                      "connection",
                                      "Send time (us)",
                                      "Receiv latency (us)",
                                  },
                                  [](Row& row, int, const auto& stats)
                                  {
                                      const auto& [remote_socket, stat] = stats;
                                      row.push_back(std::format("{}", remote_socket));
                                      row.push_back(std::format("{}", stat.connection_name));
                                      row.push_back(std::format("{} (+{})", stat.send_time, stat.send_time_offset));
                                      row.push_back(std::format("{}", stat.receiv_time - stat.send_time));
                                  });
        spdlog::debug("Performance report from sockets:\n{}", str);
    }

    void AppReport::report_frame_reading_result()
    {
        auto str = format_records(frame_reading_records_,
                                  {
                                      "Actor",
                                      "Avg. time (ns/frame)",
                                      "Avg. bytes (/frame)",
                                      "Frames",
                                  },
                                  [](Row& row, const auto& stat)
                                  {
                                      const auto avg_time =
                                          static_cast<double>(stat.total_time_ns) / static_cast<double>(stat.n_frames);

                                      const auto bytes_per_frame = static_cast<double>(stat.total_bytes_read) /
                                                                   static_cast<double>(stat.n_frames);
                                      row.push_back(std::format("{:.1f}", avg_time));
                                      row.push_back(std::format("{:.0f}", bytes_per_frame));
                                      row.push_back(std::format("{}", stat.n_frames));
                                  });
        spdlog::debug("Performance report from frame reading processes:\n{}", str);
    }

    void AppReport::report_buffer_result()
    {
        auto str = format_records(queue_record_,
                                  {
                                      "Causes of failure",
                                      "Empty trash queue",
                                      "Full trash queue",
                                      "Full valid queue",
                                  },
                                  [](Row& row, const auto& stat)
                                  {
                                      row.push_back(std::format("{}", stat.empty_trash));
                                      row.push_back(std::format("{}", stat.full_trash));
                                      row.push_back(std::format("{}", stat.full_valid));
                                  });
        spdlog::debug("Buffer Queue report:\n{}", str);
    }

    AppReport::~AppReport()
    {
        report_task_result();
        report_sink_file_result();
        report_socket_result();
        report_frame_reading_result();
        report_buffer_result();
    }
} // namespace srs
