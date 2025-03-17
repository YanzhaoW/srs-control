#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <fmt/chrono.h>
#include <fmt/color.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <srs/data/DataStructsFormat.hpp>
#include <srs/workflow/Handler.hpp>

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
        while (true)
        {
            co_await clock_.async_wait(asio::use_awaitable);
            clock_.expires_after(period_);

            auto total_bytes_count = processor_->get_read_data_bytes();
            auto total_hits_count = processor_->get_processed_hit_number();

            auto time_now = std::chrono::steady_clock::now();

            auto time_duration = std::chrono::duration_cast<std::chrono::microseconds>(time_now - last_print_time_);
            auto bytes_read = static_cast<double>(total_bytes_count - last_read_data_bytes_);
            auto hits_processed = static_cast<double>(total_hits_count - last_processed_hit_num_);

            last_read_data_bytes_ = total_bytes_count;
            last_processed_hit_num_ = total_hits_count;
            last_print_time_ = time_now;

            const auto time_duration_double = static_cast<double>(time_duration.count());
            current_received_bytes_MBps_.store(bytes_read / time_duration_double);
            current_hits_ps_.store(hits_processed / time_duration_double * 1e6);

            set_speed_string(current_received_bytes_MBps_.load());
            console_->info("Data reading rate: {}. Press \"Ctrl-C\" to stop the program.\r", speed_string_);
        }
    }

    void DataMonitor::set_speed_string(double speed_MBps)
    {
        if (speed_MBps < 1.)
        {
            speed_string_ =
                fmt::format(fg(fmt::color::yellow) | fmt::emphasis::bold, "{:>7.5} KB/s", 1000. * speed_MBps);
        }
        else
        {
            speed_string_ = fmt::format(fg(fmt::color::yellow) | fmt::emphasis::bold, "{:>7.5} MB/s", speed_MBps);
        }
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
    }

    void Handler::start(bool is_blocking)
    {
        is_stopped_.store(false);
        spdlog::debug("Workflow starts.");
        if (print_mode_ == print_speed)
        {
            monitor_.start();
        }
        analysis_loop(is_blocking);
        spdlog::trace("Workflow exit start().");
    }

    Handler::~Handler() { stop(); }
    // DataProcessor::~DataProcessor() = default;

    void Handler::stop()
    {
        // CAS operation to guarantee the thread safty
        auto expected = false;
        spdlog::trace("Try to stop the workflow. Current is_stopped status: {}", is_stopped_.load());
        if (is_stopped_.compare_exchange_strong(expected, true))
        {
            spdlog::trace("Try to stop data monitor");
            monitor_.stop();
            data_queue_.abort();
            spdlog::trace("Workflow is stopped");
        }
    }

    void Handler::read_data_once(std::span<BufferElementType> read_data)
    {
        total_read_data_bytes_ += read_data.size();
        auto is_success = data_queue_.try_emplace(read_data);
        if (not is_success)
        {
            spdlog::critical("Data queue is full and message is lost. Try to increase its capacity!");
        }
    }

    void Handler::analysis_loop(bool is_blocking)
    {
        try
        {
            spdlog::trace("entering workflow loop");
            // TODO: Use direct binary data

            while (true)
            {
                if (is_stopped_.load())
                {
                    break;
                }
                auto analysis_result = data_processes_.analysis_one(data_queue_, is_blocking);
                update_monitor();
                print_data();

                auto is_reading = app_->get_status().is_reading.load();
                if (not(is_reading or analysis_result))
                {
                    break;
                }
            }
        }
        catch (tbb::user_abort& ex)
        {
            spdlog::trace("Workflow: {}", ex.what());
        }
        catch (std::exception& ex)
        {
            spdlog::critical("Exception occured: {}", ex.what());
            app_->set_error_string(ex.what());
            // app_->exit();
        }
        spdlog::debug("Workflow loop is done.\n");
    }

    void Handler::update_monitor()
    {
        const auto& struct_data = data_processes_.get_struct_data();

        total_processed_hit_numer_ += struct_data.hit_data.size();
        monitor_.update(struct_data);
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
