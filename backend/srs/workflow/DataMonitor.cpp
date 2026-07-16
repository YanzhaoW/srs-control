#include "DataMonitor.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/ExitLogger.hpp"
#include "srs/workflow/AnalysisHandle.hpp"
#include <asio/awaitable.hpp>
#include <asio/detached.hpp>
#include <asio/error.hpp>
#include <asio/impl/co_spawn.hpp>
#include <asio/redirect_error.hpp>
#include <asio/use_awaitable.hpp>
#include <chrono>
#include <fmt/color.h>
#include <memory>
#include <spdlog/common.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <system_error>
#include <utility>

namespace srs::workflow
{
    DataMonitor::DataMonitor(AnalysisHandle* analysis_handle, io_context_type* io_context)
        : analysis_handle_{ analysis_handle }
        , io_context_{ io_context }
        , clock_{ *io_context_ }
    {
    }

    auto DataMonitor::print_cycle() -> asio::awaitable<void>
    {
        auto _ = ExitLogger{};
        clock_.expires_after(period_);
        console_ = spdlog::stdout_color_st("Data Monitor");
        auto console_format = std::make_unique<spdlog::pattern_formatter>(
            "[%H:%M:%S] <%n> %v", spdlog::pattern_time_type::local, std::string(""));
        console_->set_formatter(std::move(console_format));
        console_->flush_on(spdlog::level::info);
        console_->set_level(spdlog::level::info);
        const auto buffer_size = analysis_handle_->get_app().get_config().data_buffer_size;
        const auto& task_workflow = analysis_handle_->get_data_workflow();
        [[maybe_unused]] const auto* frame_count_checker =
            analysis_handle_->get_sink_manager().get_frame_count_checker();
        auto error_code = std::error_code{};
        while (not is_cancelled_.load())
        {
            co_await clock_.async_wait(asio::redirect_error(asio::use_awaitable, error_code));
            if (error_code == asio::error::operation_aborted)
            {
                break;
            }
            clock_.expires_after(period_);

            auto read_total_bytes_count = analysis_handle_->get_read_data_bytes();
            auto read_total_hits_count = analysis_handle_->get_processed_hit_number();
            auto frame_counts = analysis_handle_->get_frame_counts();
            auto write_total_bytes_count = task_workflow.get_data_bytes();
            auto drop_total_bytes_count = analysis_handle_->get_drop_data_bytes();

            auto time_now = std::chrono::steady_clock::now();
            auto time_duration = std::chrono::duration_cast<std::chrono::microseconds>(time_now - last_print_time_);

            auto bytes_read = static_cast<double>(read_total_bytes_count - last_read_data_bytes_);
            auto bytes_write = static_cast<double>(write_total_bytes_count - last_write_data_bytes_);
            auto bytes_drop = static_cast<double>(drop_total_bytes_count - last_drop_data_bytes_);
            auto hits_processed = static_cast<double>(read_total_hits_count - last_processed_hit_num_);
            auto frame_counts_diff = frame_counts - last_frame_counts_;

            last_read_data_bytes_ = read_total_bytes_count;
            last_write_data_bytes_ = write_total_bytes_count;
            last_drop_data_bytes_ = drop_total_bytes_count;
            last_processed_hit_num_ = read_total_hits_count;
            last_frame_counts_ = frame_counts;
            last_print_time_ = time_now;

            const auto time_duration_us = static_cast<double>(time_duration.count());
            current_received_bytes_MBps_ = bytes_read / time_duration_us;
            current_write_bytes_MBps_ = bytes_write / time_duration_us;
            current_drop_bytes_MBps_ = bytes_drop / time_duration_us;
            current_hits_ps_ = hits_processed / time_duration_us * 1e6;
            const auto frame_rate = (frame_counts_diff == 0) ? 0.
                                                             : bytes_read / static_cast<double>(frame_counts_diff) /
                                                                   static_cast<double>(buffer_size) * 100.;

            set_speed_string();
            console_->info("read (buf)|write|drop rate: {} ({:>2.0f}%) | {} | {} {}. \r",
                           read_speed_string_,
                           frame_rate,
                           write_speed_string_,
                           drop_speed_string_,
                           unit_string_);
        }
    }

    void DataMonitor::set_speed_string()
    {
        const auto read_speed_MBps = current_received_bytes_MBps_;
        const auto write_speed_MBps = current_write_bytes_MBps_;
        const auto drop_speed_MBps = current_drop_bytes_MBps_;

        const auto scale = (read_speed_MBps < 1.) ? 1000. : 1.;
        unit_string_ = (read_speed_MBps < 1.) ? fmt::format(fmt::emphasis::bold, "KB/s")
                                              : fmt::format(fmt::emphasis::bold, "MB/s");
        read_speed_string_ =
            fmt::format(fg(fmt::color::yellow) | fmt::emphasis::bold, "{:>7.5}", scale * read_speed_MBps);
        write_speed_string_ =
            fmt::format(fg(fmt::color::spring_green) | fmt::emphasis::bold, "{:^7.5}", scale * write_speed_MBps);
        drop_speed_string_ =
            fmt::format(fg(fmt::color::orange_red) | fmt::emphasis::bold, "{:<7.5}", scale * drop_speed_MBps);
    }

    void DataMonitor::start() { asio::co_spawn(*io_context_, print_cycle(), asio::detached); }

    void DataMonitor::stop()
    {
        auto _ = ExitLogger{};
        clock_.cancel();
        is_cancelled_.store(true);
        spdlog::debug("DataMonitor: rate polling clock is killed.");
    }
} // namespace srs::workflow
