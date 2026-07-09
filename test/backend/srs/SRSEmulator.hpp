#pragma once

#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/readers/RawFrameReader.hpp"
#include "srs/utils/CommonAlias.hpp" // IWYU pragma: keep
#include <asio.hpp>
#include <asio/any_io_executor.hpp>
#include <asio/awaitable.hpp>
#include <asio/deferred.hpp>
#include <asio/impl/co_spawn.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/basic_endpoint.hpp>
#include <asio/ip/udp.hpp>
#include <asio/steady_timer.hpp>
#include <asio/system_timer.hpp>
#include <asio/thread_pool.hpp>
#include <asio/uses_executor.hpp>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace srs::test
{
    class SRSEmulator
    {
      public:
        using IOContextType = asio::thread_pool;
        struct Config
        {
            int data_port = 0;
            int listen_port = 0;
            int write_port = 0;
            int server_idx = 0;
            std::size_t n_servers = 1;
            std::string_view filename;
            std::string ip;
        };
        enum class ReceiveType : uint8_t
        {
            invalid,
            acq_on,
            acq_off,
        };

        void set_delay_time(std::size_t time) { delay_time_ = std::chrono::nanoseconds{ time }; }
        explicit SRSEmulator(const Config& config, IOContextType& io_context);
        void set_continue_output(bool is_continue) { is_continue_.store(is_continue); }
        void wait_for_connection();
        void wait_for_data_sender();

        auto get_config() const -> const auto& { return config_; }

        auto get_n_frames_sent() const -> const auto& { return n_frames_sent_; }

      private:
        std::chrono::nanoseconds delay_time_{ 3 };
        std::atomic<bool> is_continue_ = false;
        std::atomic<bool> is_idle_ = true;
        std::atomic<bool> is_shutdown_ = false;
        gsl::not_null<IOContextType*> io_context_;
        std::string source_filename_;
        std::size_t n_frames_sent_ = 0;
        Config config_;
        reader::RawFrame frame_reader_;
        asio::ip::udp::socket udp_socket_;
        std::shared_ptr<asio::system_timer> data_sender_wait_timer_ =
            std::make_shared<asio::system_timer>(*io_context_);
        asio::steady_timer data_sending_control_{ *io_context_ };

        auto start_send_data() -> asio::awaitable<void>;

        void do_if_acq_on(asio::any_io_executor& executor);

        void do_if_acq_off();

        void listen_on_communications();
        // auto cancel_coroutine() -> cobalt::promise<void>;

        auto listen_coro() -> asio::awaitable<void>;
        auto send_response(connection::udp::endpoint endpoint, ReceiveType result_type) -> asio::awaitable<void>;
    };
} // namespace srs::test
