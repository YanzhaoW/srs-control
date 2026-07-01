#include "srs/workflow/Handler.hpp"
#include "srs/Application.hpp"
#include "srs/data/BufferQueue.hpp"
#include "srs/data/DataStructsFormat.hpp"
#include "srs/data/LargeBuffer.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/ExitLogger.hpp"
#include "srs/workflow/TaskDiagram.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/impl/co_spawn.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fmt/base.h>
#include <fmt/color.h>
#include <memory>
#include <mutex>
#include <print>
#include <spdlog/common.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#ifdef USE_ONEAPI_TBB
#include <oneapi/tbb/concurrent_queue.h>
#else
#include <tbb/concurrent_queue.h>
#endif

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <utility>

namespace srs::workflow
{
    DataMonitor::DataMonitor(Handler* processor, io_context_type* io_context)
        : processor_{ processor }
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
        const auto buffer_size = processor_->get_app().get_config().data_buffer_size;
        const auto& task_workflow = processor_->get_data_workflow();
        auto error_code = boost::system::error_code{};
        while (not is_cancelled_.load())
        {
            co_await clock_.async_wait(asio::redirect_error(asio::use_awaitable, error_code));
            if (error_code == boost::asio::error::operation_aborted)
            {
                break;
            }
            clock_.expires_after(period_);

            auto read_total_bytes_count = processor_->get_read_data_bytes();
            auto read_total_hits_count = processor_->get_processed_hit_number();
            auto frame_counts = processor_->get_frame_counts();
            auto write_total_bytes_count = task_workflow.get_data_bytes();
            auto drop_total_bytes_count = processor_->get_drop_data_bytes();

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
            console_->info("read (buf)|write|drop rate: {} ({:>2.0f}%) | {} | {} {}. Press \"Ctrl-C\" to stop \r",
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

    Handler::Handler(App* control, std::size_t n_lines)
        : is_data_drop_warn_{ control->get_config().warn_if_data_drop }
        , n_lines_{ n_lines }
        , app_{ control }
        , monitor_{ this, &(control->get_io_context()) }
        , buffer_queue_{ BufferQueue{ { .buffer_size = app_->get_config().data_buffer_size,
                                        .reserve_size = control->get_config().buffer_queue_capacity / 2,
                                        .queue_capacity = control->get_config().buffer_queue_capacity } } }
    {
        spdlog::debug("Handler: Setting the capacity of the buffer queue to {}",
                      control->get_config().buffer_queue_capacity);
    }

    void Handler::start()
    {
        auto _ = ExitLogger{};
        is_stopped_.store(false);
        task_diagram_ = std::make_unique<TaskDiagram>(this, n_lines_);

        if (print_mode_ == print_speed)
        {
            monitor_.start();
        }

        cv_.notify_all();
        try
        {
            spdlog::trace("Analysis workflow: entering workflow loop");
            // TODO: Use direct binary data

            task_diagram_->construct_taskflow_and_run(buffer_queue_, is_stopped_);
        }
        catch (tbb::user_abort& ex)
        {
            spdlog::trace("Analysis workflow: {}", ex.what());
        }
    }

    Handler::~Handler()
    {
        // stop();
        spdlog::debug("Analysis workflow: total read data bytes: {} ", total_read_data_bytes_.load());
        spdlog::debug("Analysis workflow: total frame counts: {} ", total_frame_counts_.load());
    }
    // DataProcessor::~DataProcessor() = default;

    void Handler::stop()
    {
        auto _ = ExitLogger{};
        // stop may be called before start, where the task_diagram_ is constructed. To prevent task_diagram_ to be
        // nullptr, wait for a condition variable, notified by the start function.
        auto lock = std::unique_lock{ cv_mutex_ };
        cv_.wait(lock, [this]() { return task_diagram_ != nullptr; });

        // CAS operation to guarantee the thread safety
        auto expected = false;
        spdlog::trace("Analysis workflow: trying to stop ... ");
        spdlog::trace("Analysis workflow: current is_stopped status: {}", is_stopped_.load());
        if (is_stopped_.compare_exchange_strong(expected, true))
        {
            const auto _ = ExitLogger{ "scope" };
            monitor_.stop();

            while (not task_diagram_->is_taskflow_abort_ready())
            {
                // spdlog::trace("waiting for task diagram to be abort ready");
            }
            buffer_queue_.abort();
        }
    }

    auto Handler::get_data_workflow() const -> const TaskDiagram&
    {
        if (task_diagram_ == nullptr)
        {
            throw std::runtime_error("task_diagram is still nullptr");
        }
        return *task_diagram_;
    }

    void Handler::read_data_once(LargeBuffer& read_data)
    {
        const auto data_size = read_data.get_size();
        total_read_data_bytes_ += data_size;
        ++total_frame_counts_;

        time_point_ = clock_.now();
        auto is_success = buffer_queue_.try_emplace(read_data);
        total_time_ns_ += static_cast<uint64_t>((clock_.now() - time_point_).count());

        if (not is_success)
        {
            total_drop_data_bytes_ += data_size;
            if (is_data_drop_warn_)
            {
                spdlog::warn("Data drop as the buffer queue is full: Current size/capacity: {}/{}.",
                             buffer_queue_.size(),
                             buffer_queue_.capacity());
            }
        }
    }
} // namespace srs::workflow
