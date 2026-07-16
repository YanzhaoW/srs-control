#pragma once

#include "srs/data/BufferQueue.hpp"
#include "srs/data/LargeBuffer.hpp"
#include "srs/sinks/Manager.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/workflow/DataMonitor.hpp"
#include "srs/workflow/FrameMissMonitor.hpp"
#include "srs/workflow/TaskDiagram.hpp"
#include <asio/any_io_executor.hpp>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace srs
{
    class App;
}

namespace srs::workflow
{
    class AnalysisHandle
    {
      public:
        explicit AnalysisHandle(App* control, std::size_t n_lines = 1);

        AnalysisHandle(const AnalysisHandle&) = delete;
        AnalysisHandle(AnalysisHandle&&) = delete;
        AnalysisHandle& operator=(const AnalysisHandle&) = delete;
        AnalysisHandle& operator=(AnalysisHandle&&) = delete;
        ~AnalysisHandle();

        // From socket interface. Need to be fast return
        void read_data_once(LargeBuffer& read_data, BufferQueue::Token& token);

        void start(asio::any_io_executor executor);

        // getters:
        [[nodiscard]] auto get_read_data_bytes() const -> uint64_t { return total_read_data_bytes_.load(); }
        [[nodiscard]] auto get_processed_hit_number() const -> uint64_t { return total_processed_hit_numer_.load(); }
        [[nodiscard]] auto get_drop_data_bytes() const -> uint64_t { return total_drop_data_bytes_.load(); }
        [[nodiscard]] auto get_frame_counts() const -> uint64_t { return total_frame_counts_.load(); }
        [[nodiscard]] auto get_data_monitor() const -> const auto& { return monitor_; }
        [[nodiscard]] auto get_data_workflow() const -> const TaskDiagram&;
        [[nodiscard]] auto get_n_lines() const -> auto { return n_lines_; }
        [[nodiscard]] auto get_sink_manager() const -> const auto& { return writers_; }
        [[nodiscard]] auto get_buffer_queue() const -> const auto& { return buffer_queue_; }
        [[nodiscard]] auto get_app() const -> const auto& { return *app_; }

        [[nodiscard]] auto get_app_ref() -> auto& { return *app_; }
        [[nodiscard]] auto get_sink_manager_ref() -> auto& { return writers_; }

        [[nodiscard]] auto get_queue_producer_token() -> BufferQueue::Token
        {
            return buffer_queue_.get_producer_token();
        }
        [[nodiscard]] auto get_queue_consumer_token() -> BufferQueue::Token
        {
            return buffer_queue_.get_consumer_token();
        }

        void print_statistics();

        // setters:
        void set_print_mode(common::DataPrintMode mode) { print_mode_ = mode; }
        void set_show_data_speed(bool val = true) { monitor_.show_data_speed(val); }
        void set_monitor_display_period(std::chrono::milliseconds duration) { monitor_.set_display_period(duration); }
        void set_output_filenames(const std::vector<std::string>& filenames)
        {
            writers_.set_output_filenames(filenames);
        }

        void stop_monitor()
        {
            monitor_.stop();
            if (frame_counter_monitor_ != nullptr)
            {
                frame_counter_monitor_->stop();
            }
        }
        void stop_workflow();

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
        sink::Manager writers_{ this };
        DataMonitor monitor_;
        std::unique_ptr<FrameMissMonitor> frame_counter_monitor_;

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
