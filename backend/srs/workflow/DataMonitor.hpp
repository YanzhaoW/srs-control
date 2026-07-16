#pragma once

#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include <asio/awaitable.hpp>
#include <asio/steady_timer.hpp>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <spdlog/logger.h>
#include <string>

namespace srs::sink
{
    class FrameCountChecker;
}

namespace srs::workflow
{
    class AnalysisHandle;

    class DataMonitor
    {
      public:
        explicit DataMonitor(AnalysisHandle* analysis_handle, io_context_type* io_context);
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
        gsl::not_null<AnalysisHandle*> analysis_handle_;
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
} // namespace srs::workflow
