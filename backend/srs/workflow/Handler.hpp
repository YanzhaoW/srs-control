#pragma once

#include <atomic>
#include <chrono>
#include <span>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <gsl/gsl-lite.hpp>
#include <spdlog/logger.h>

#include <srs/workflow/TaskDiagram.hpp>

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
        void update(const StructData& data_struct) {}
        void http_server_loop();

        // getters:
        [[nodiscard]] auto get_received_bytes_MBps() const -> double { return current_received_bytes_MBps_.load(); }
        [[nodiscard]] auto get_processed_hit_rate() const -> double { return current_hits_ps_.load(); }

      private:
        bool is_shown_ = true;
        gsl::not_null<Handler*> processor_;
        gsl::not_null<io_context_type*> io_context_;
        std::shared_ptr<spdlog::logger> console_;
        asio::steady_timer clock_;
        std::atomic<uint64_t> last_read_data_bytes_ = 0;
        std::atomic<uint64_t> last_processed_hit_num_ = 0;
        std::chrono::time_point<std::chrono::steady_clock> last_print_time_ = std::chrono::steady_clock::now();
        std::chrono::milliseconds period_ = common::DEFAULT_DISPLAY_PERIOD;
        std::atomic<double> current_received_bytes_MBps_;
        std::atomic<double> current_hits_ps_;
        std::string speed_string_;

        void set_speed_string(double speed_MBps);
        auto print_cycle() -> asio::awaitable<void>;
    };

    class Handler
    {
      public:
        explicit Handler(App* control);

        Handler(const Handler&) = delete;
        Handler(Handler&&) = delete;
        Handler& operator=(const Handler&) = delete;
        Handler& operator=(Handler&&) = delete;
        ~Handler();

        // From socket interface. Need to be fast return
        void read_data_once(std::span<BufferElementType> read_data);

        void abort() { data_queue_.abort(); }

        void start(bool is_blocking);

        // getters:
        [[nodiscard]] auto get_read_data_bytes() const -> uint64_t { return total_read_data_bytes_.load(); }
        [[nodiscard]] auto get_processed_hit_number() const -> uint64_t { return total_processed_hit_numer_.load(); }
        // [[nodiscard]] auto get_export_data() -> auto& { return struct_serializer.get_output_data(); }
        [[nodiscard]] auto get_data_monitor() const -> const auto& { return monitor_; }
        [[nodiscard]] auto get_app() -> auto& { return *app_; }

        // setters:
        void set_print_mode(common::DataPrintMode mode) { print_mode_ = mode; }
        void set_show_data_speed(bool val = true) { monitor_.show_data_speed(val); }
        void set_monitor_display_period(std::chrono::milliseconds duration) { monitor_.set_display_period(duration); }
        void set_output_filenames(const std::vector<std::string>& filenames)
        {
            data_processes_.set_output_filenames(filenames);
        }

        void stop();

      private:
        using enum common::DataPrintMode;

        std::atomic<bool> is_stopped_{ false };
        std::size_t received_data_size_{};
        common::DataPrintMode print_mode_ = common::DataPrintMode::print_speed;
        std::atomic<uint64_t> total_read_data_bytes_ = 0;
        std::atomic<uint64_t> total_processed_hit_numer_ = 0;
        gsl::not_null<App*> app_;
        DataMonitor monitor_;

        // Data buffer
        tbb::concurrent_bounded_queue<process::SerializableMsgBuffer> data_queue_;
        TaskDiagram data_processes_;

        // should run on a different task
        void analysis_loop(bool is_blocking);
        void update_monitor();
        void write_data();
        void print_data();
        void clear_data_buffer();
    };

} // namespace srs::workflow
