#pragma once

#include "srs/Application.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/readers/RawFrameReader.hpp"
#include "srs/writers/UDPWriter.hpp"
#include <atomic>
#include <boost/asio.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/impl/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/uses_executor.hpp>
#include <boost/cobalt/promise.hpp>
#include <boost/cobalt/task.hpp>
#include <boost/cobalt/this_thread.hpp>
#include <boost/thread/future.hpp>
#include <chrono>
#include <cstdint>
#include <future>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <utility>

namespace srs::test
{
    class SRSEmulator
    {
      public:
        struct Config
        {
            int data_port = 0;
            int listen_port = 0;
            int write_port = 0;
            std::string_view filename;
        };
        enum class ReceiveType : uint8_t
        {
            invalid,
            acq_on,
            acq_off,
        };

        explicit SRSEmulator(const Config& config, App& app);
        void set_continue_output(bool is_continue) { is_continue_.store(is_continue); }
        void wait_for_connection();
        void wait_for_data_sender();

        // Getters
        auto get_remote_endpoint() -> const auto& { return udp_writer_.get_remote_endpoint(); }

      private:
        using IOContextType = asio::thread_pool;

        std::atomic<bool> is_continue_ = false;
        std::atomic<bool> is_idle_ = true;
        std::atomic<bool> is_shutdown_ = false;
        IOContextType io_context_{ 4 };
        std::string source_filename_;
        Config config_;
        reader::RawFrame frame_reader_;
        writer::UDP udp_writer_;
        asio::ip::udp::socket udp_socket_;
        std::shared_ptr<asio::system_timer> data_sender_status_ = std::make_shared<asio::system_timer>(io_context_);
        asio::steady_timer data_sending_control_{ io_context_ };

        auto start_send_data() -> asio::awaitable<void>;

        void do_if_acq_on(asio::any_io_executor& executor);

        void do_if_acq_off();

        void listen_on_communications();
        // auto cancel_coroutine() -> cobalt::promise<void>;

        auto listen_coro() -> asio::awaitable<void>;
        auto send_response(connection::udp::endpoint endpoint, ReceiveType result_type) -> asio::awaitable<void>;
    };
} // namespace srs::test
