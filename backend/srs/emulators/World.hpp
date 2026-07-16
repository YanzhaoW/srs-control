#pragma once

#include "srs/emulators/FECEmulator.hpp"
#include <asio/thread_pool.hpp>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace srs::emulator
{
    /**
     * @brief Emulators for FECs devices
     */
    class World
    {
      public:
        struct Config
        {
            std::size_t frame_wait_time_ns = 1000;
            std::size_t n_threads = 4;
            bool non_stop = true;
            std::vector<FECEmulator::Config> FECs{ FECEmulator::Config{ .ip = "127.0.0.1" } };
        };

        World(const Config& config)
            : config_{ config }
            , io_context_{ config_.n_threads }
        {
        }

        void init();

        void launch();

        auto request_frame_counter() -> uint32_t { return frame_counter_++; }
        void reset_frame_counter() { return frame_counter_.store(0); }

        auto get_config() const -> const Config& { return config_; }

      private:
        Config config_;
        std::atomic<uint32_t> frame_counter_{ 0 };
        asio::thread_pool io_context_;
        std::vector<std::unique_ptr<FECEmulator>> fec_servers_;
    };
} // namespace srs::emulator
