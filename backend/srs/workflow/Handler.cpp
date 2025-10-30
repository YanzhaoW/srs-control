#include "srs/workflow/Handler.hpp"
#include "srs/data/DataStructsFormat.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/workflow/TaskDiagram.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/impl/co_spawn.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <chrono>
#include <exception>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/ranges.h>
#include <memory>
#include <oneapi/tbb/concurrent_queue.h>
#include <span>
#include <spdlog/common.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <spdlog/spdlog.h>
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
        clock_.expires_after(period_);
        console_ = spdlog::stdout_color_st("Data Monitor");
        auto console_format = std::make_unique<spdlog::pattern_formatter>(
            "[%H:%M:%S] <%n> %v", spdlog::pattern_time_type::local, std::string(""));
        console_->set_formatter(std::move(console_format));
        console_->flush_on(spdlog::level::info);
        console_->set_level(spdlog::level::info);
        const auto buffer_size = processor_->get_app().get_config().data_buffer_size;
        const auto& task_workflow = processor_->get_data_workflow();
        while (true)
        {
            co_await clock_.async_wait(asio::use_awaitable);
            clock_.expires_after(period_);

            auto read_total_bytes_count = processor_->get_read_data_bytes();
            auto read_total_hits_count = processor_->get_processed_hit_number();
            auto frame_counts = processor_->get_frame_counts();
            auto write_total_bytes_count = task_workflow.get_data_bytes();
            auto drop_total_bytes_count = task_workflow.get_drop_data_bytes();

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
            console_->info("read|write|drop rate: {} ({:>2.0f}%) | {} | {} {}. Press \"Ctrl-C\" to stop the program \r",
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
            fmt::format(fg(fmt::color::orange_red) | fmt::emphasis::bold, "{:<4.2}", scale * drop_speed_MBps);
    }

    void DataMonitor::start() { asio::co_spawn(*io_context_, print_cycle(), asio::detached); }

    void DataMonitor::stop()
    {
        clock_.cancel();
        spdlog::debug("DataMonitor: rate polling clock is killed.");
    }

    Handler::Handler(App* control)
        : app_{ control }
        , monitor_{ this, &(control->get_io_context()) }
        , data_processes_{ this, control->get_io_context() }
    {
        data_queue_.set_capacity(common::DEFAULT_DATA_QUEUE_SIZE);
    }

    void Handler::start(bool is_blocking)
    {
        is_stopped_.store(false);
        data_processes_.init();
        spdlog::debug("Analysis workflow: main loop starts.");
        if (print_mode_ == print_speed)
        {
            monitor_.start();
        }
        analysis_loop(is_blocking);
        spdlog::trace("Analysis workflow: exiting the main loop.");
    }

    Handler::~Handler()
    {
        stop();
        spdlog::debug("Analysis workflow: total read data bytes: {} ", total_read_data_bytes_.load());
        spdlog::debug("Analysis workflow: total frame counts: {} ", total_frame_counts_.load());
    }
    // DataProcessor::~DataProcessor() = default;

    void Handler::stop()
    {
        // CAS operation to guarantee the thread safety
        auto expected = false;
        spdlog::debug("Analysis workflow: trying to stop ... ");
        spdlog::trace("Analysis workflow: current is_stopped status: {}", is_stopped_.load());
        if (is_stopped_.compare_exchange_strong(expected, true))
        {
            spdlog::trace("Analysis workflow: Try to stop data monitor");
            monitor_.stop();
            data_queue_.abort();
            spdlog::trace("Analysis workflow: successfully stopped");
        }
    }

    void Handler::read_data_once(std::span<BufferElementType> read_data)
    {
        total_read_data_bytes_ += read_data.size();
        ++total_frame_counts_;
        auto is_success = data_queue_.try_emplace(read_data);
        if (not is_success)
        {
            spdlog::trace("Analysis workflow: Data queue is full and message is lost. Try to increase its capacity!");
        }
    }

    void Handler::analysis_loop(bool is_blocking)
    {
        try
        {
            spdlog::trace("Analysis workflow: entering workflow loop");
            // TODO: Use direct binary data

            while (true)
            {
                if (is_stopped_.load())
                {
                    break;
                }
                [[maybe_unused]] auto analysis_result = data_processes_.analysis_one(data_queue_, is_blocking);
                update_monitor();
                print_data();

                // auto is_reading = app_->get_status().is_reading.load();
                // if (not(is_reading or analysis_result))
                // {
                //     break;
                // }
            }
        }
        catch (tbb::user_abort& ex)
        {
            spdlog::trace("Analysis workflow: {}", ex.what());
        }
        catch (std::exception& ex)
        {
            spdlog::critical("Exception occurred: {}", ex.what());
            app_->set_error_string(ex.what());
            // app_->exit();
        }
        spdlog::debug("Analysis workflow: Workflow loop is done.\n");
    }

    void Handler::update_monitor()
    {
        const auto& struct_data = data_processes_.get_struct_data();

        total_processed_hit_numer_ += struct_data.hit_data.size();
        // monitor_.update(struct_data);
    }

    void Handler::print_data()
    {
        const auto& export_data = data_processes_.get_struct_data();
        const auto& raw_data = data_processes_.get_data<TaskDiagram::raw>();
        if (print_mode_ == print_raw)
        {
            spdlog::info("data: {:x}", fmt::join(raw_data, ""));
        }
        if (print_mode_ == print_header or print_mode_ == print_raw or print_mode_ == print_all)
        {
            spdlog::info("{}. Data size: {}", export_data.header, received_data_size_);
        }

        if (print_mode_ == print_all)
        {
            for (const auto& hit_data : export_data.hit_data)
            {
                spdlog::info("{}", hit_data);
            }
            for (const auto& marker_data : export_data.marker_data)
            {
                spdlog::info("{}", marker_data);
            }
        }
    }
} // namespace srs::workflow
