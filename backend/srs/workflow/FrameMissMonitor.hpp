#pragma once

#include "srs/sinks/FrameCountChecker.hpp"
#include "srs/utils/ExitLogger.hpp"
#include <algorithm>
#include <asio/any_io_executor.hpp>
#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/impl/co_spawn.hpp>
#include <asio/steady_timer.hpp>
#include <asio/this_coro.hpp>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <gsl/gsl-lite.hpp>
#include <spdlog/spdlog.h>
#include <vector>

namespace srs::workflow
{
    class FrameMissMonitor
    {
      public:
        struct Config
        {
            std::size_t sample_size{};
            int sample_time_ms{};
        };
        explicit FrameMissMonitor(const Config& config, const sink::FrameCountChecker& analysis_handle)
            : config_{ config }
            , frame_count_checker{ &analysis_handle }
        {
            samples_.reserve(config_.sample_size);
        }

        void launch(asio::any_io_executor executor) { asio::co_spawn(executor, main_loop(), asio::detached); }

        void stop() { is_stopped_.store(true); }

      private:
        std::atomic<bool> is_stopped_{ false };
        Config config_;
        std::vector<int64_t> samples_;
        double last_average_drop_count = 0.;
        gsl::not_null<const sink::FrameCountChecker*> frame_count_checker;

        void report()
        {
            const auto average = static_cast<double>(std::ranges::fold_left(samples_, int64_t{ 0 }, std::plus{})) /
                                 static_cast<double>(samples_.size());

            if (auto diff = average - last_average_drop_count; diff > 1.)
            {
                spdlog::warn(
                    "{:.0f} frames miss detected from the frame counter in the last {:.1f} seconds",
                    diff,
                    static_cast<double>(config_.sample_size) * static_cast<double>(config_.sample_time_ms) / 1000.);
            }
            last_average_drop_count = average;
            samples_.clear();
        }

        auto main_loop() -> asio::awaitable<void>
        {
            const auto _ = ExitLogger{};
            auto executor = co_await asio::this_coro::executor;
            auto sample_clock = asio::steady_timer{ executor };
            while (not is_stopped_.load())
            {
                sample_clock.expires_after(std::chrono::milliseconds(config_.sample_time_ms));
                samples_.push_back(frame_count_checker->get_frame_drop_count());

                if (samples_.size() == config_.sample_size)
                {
                    report();
                }
            }
        }
    };
}; // namespace srs::workflow
