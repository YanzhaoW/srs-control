#pragma once

#include "srs/data/BufferQueue.hpp"
#include "srs/data/LargeBuffer.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/workflow/TaskDiagram.hpp"
#include "srs/writers/DataWriter.hpp"
#include <atomic>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <mutex>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

namespace srs
{
    class App;
}

namespace srs::workflow
{
    class Handler;

    class DataMonitor
    {
      public:
        explicit DataMonitor(Handler* processor, io_context_type* io_context);
        void show_data_speed(bool val = true) { is_shown_ = val; }
        void set_display_period(std::chrono::milliseconds duration) { period_ = duration; }
        void start();
        void stop();
        // void update(const StructData& data_struct) {}
        void http_server_loop();

        // getters:
        [[nodiscard]] auto get_received_bytes_MBps() const -> double { return current_received_bytes_MBps_; }
        [[nodiscard]] auto get_processed_hit_rate() const -> double { return current_hits_ps_; }

      private:
        bool is_shown_ = true;
        std::atomic<bool> is_cancelled_ = false;
        gsl::not_null<Handler*> processor_;
        gsl::not_null<io_context_type*> io_context_;
        std::shared_ptr<spdlog::logger> console_;
        asio::steady_timer clock_;
        std::chrono::time_point<std::chrono::steady_clock> last_print_time_ = std::chrono::steady_clock::now();
        std::chrono::milliseconds period_ = common::DEFAULT_DISPLAY_PERIOD;
        uint64_t last_read_data_bytes_ = 0;
        uint64_t last_write_data_bytes_ = 0;
        uint64_t last_drop_data_bytes_ = 0;
        uint64_t last_processed_hit_num_ = 0;
        uint64_t last_frame_counts_ = 0;
        double current_received_bytes_MBps_ = 0.;
        double current_write_bytes_MBps_ = 0.;
        double current_drop_bytes_MBps_ = 0.;
        double current_hits_ps_ = 0.;
        std::string read_speed_string_;
        std::string write_speed_string_;
        std::string drop_speed_string_;
        std::string unit_string_;

        void set_speed_string();
        auto print_cycle() -> asio::awaitable<void>;
    };

    class Handler
    {
      public:
        explicit Handler(App* control, std::size_t n_lines = 1);

        Handler(const Handler&) = delete;
        Handler(Handler&&) = delete;
        Handler& operator=(const Handler&) = delete;
        Handler& operator=(Handler&&) = delete;
        ~Handler();

        // From socket interface. Need to be fast return
        void read_data_once(LargeBuffer& read_data, BufferQueue::Token& token);

        void start();

        // getters:
        [[nodiscard]] auto get_read_data_bytes() const -> uint64_t { return total_read_data_bytes_.load(); }
        [[nodiscard]] auto get_processed_hit_number() const -> uint64_t { return total_processed_hit_numer_.load(); }
        [[nodiscard]] auto get_drop_data_bytes() const -> uint64_t { return total_drop_data_bytes_.load(); }
        [[nodiscard]] auto get_frame_counts() const -> uint64_t { return total_frame_counts_.load(); }
        [[nodiscard]] auto get_data_monitor() const -> const auto& { return monitor_; }
        [[nodiscard]] auto get_data_workflow() const -> const TaskDiagram&;
        [[nodiscard]] auto get_app() -> auto& { return *app_; }
        [[nodiscard]] auto get_n_lines() const -> auto { return n_lines_; }
        auto get_writer() -> auto* { return &writers_; }
        auto get_buffer_queue() const -> const auto& { return buffer_queue_; }

        [[nodiscard]] auto get_queue_producer_token() -> BufferQueue::Token
        {
            return buffer_queue_.get_producer_token();
        }
        [[nodiscard]] auto get_queue_consumer_token() -> BufferQueue::Token
        {
            return buffer_queue_.get_consumer_token();
        }

        void print_statistics()
        {
            spdlog::trace("statistics from the handler \n\t Average time per frame: {:.2f} ns\n\t Average bytes per "
                          "frame: {:.0f}",
                          get_average_ns_time_on_push(),
                          get_average_byte_per_frame());
            spdlog::trace("statistics from the buffer queue \n\tAttempt failed due to empty trash: {}\n\tAttempt "
                          "failed due to full trash:: {}",
                          buffer_queue_.get_n_trash_recycle_failures_empty(),
                          buffer_queue_.get_n_trash_recycle_failures_full());
        }

        // setters:
        void set_n_pipelines(std::size_t n_lines) { n_lines_ = n_lines; }
        void set_print_mode(common::DataPrintMode mode) { print_mode_ = mode; }
        void set_show_data_speed(bool val = true) { monitor_.show_data_speed(val); }
        void set_monitor_display_period(std::chrono::milliseconds duration) { monitor_.set_display_period(duration); }
        void set_output_filenames(const std::vector<std::string>& filenames)
        {
            writers_.set_output_filenames(filenames);
        }

        void stop();

      private:
        using enum common::DataPrintMode;

        bool is_data_drop_warn_ = false;
        std::size_t n_lines_ = 1;
        std::atomic<bool> is_stopped_{ true };
        std::size_t received_data_size_{};
        common::DataPrintMode print_mode_ = common::DataPrintMode::print_speed;
        std::atomic<uint64_t> total_read_data_bytes_ = 0;
        std::atomic<uint64_t> total_drop_data_bytes_ = 0;
        std::atomic<uint64_t> total_processed_hit_numer_ = 0;
        std::atomic<uint64_t> total_frame_counts_ = 0;
        gsl::not_null<App*> app_;
        writer::Manager writers_{ this };
        DataMonitor monitor_;

        // mutex and cv make sure the stop always wait for start
        std::mutex cv_mutex_;
        std::condition_variable cv_;

        // Data buffer
        BufferQueue buffer_queue_;
        std::unique_ptr<TaskDiagram> task_diagram_ = nullptr;

        std::chrono::steady_clock clock_;
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> time_point_;
        std::uint64_t total_time_ns_ = 0;

        void clear_data_buffer();

        auto get_average_ns_time_on_push() -> double
        {
            return static_cast<double>(total_time_ns_) / static_cast<double>(total_frame_counts_);
        }

        auto get_average_byte_per_frame() -> double
        {
            return static_cast<double>(total_read_data_bytes_) / static_cast<double>(total_frame_counts_);
        }
    };

} // namespace srs::workflow
