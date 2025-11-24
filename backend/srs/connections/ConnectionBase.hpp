#pragma once

#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/FecSwitchSocket.hpp"
#include "srs/converters/SerializableBuffer.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonConcepts.hpp"
#include "srs/utils/CommonDefinitions.hpp"
// #include "srs/utils/CommonFunctions.hpp"
#include "srs/utils/UDPFormatters.hpp" // IWYU pragma: keep
#include <algorithm>
#include <atomic>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/cancellation_condition.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>
// #include <chrono>
// #include <csignal>
#include <cstddef>
#include <cstdint>
#include <fmt/base.h>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <optional>
#include <span>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <utility>
// #include <variant>
#include <vector>

namespace srs::connection
{
    class FecSwitchSocket;
    // derive from enable_shared_from_this to make sure object still alive in the coroutine
    class Base : public std::enable_shared_from_this<Base>
    {
      public:
        struct Config
        {
            std::string name;
            std::size_t buffer_size = common::SMALL_READ_MSG_BUFFER_SIZE;
        };
        explicit Base(const Config& config);

        // possible overload from derived class
        void read_data_handle(std::span<BufferElementType> read_data);

        void close() { close_socket(); }
        static void on_fail() { spdlog::debug("default on_fail is called!"); }

        // TODO: utilize listen function
        void communicate(this auto&& self,
                         const std::vector<CommunicateEntryType>& data,
                         uint16_t address,
                         std::shared_ptr<FecSwitchSocket> socket)
        {
            self.encode_write_msg(self.write_msg_buffer_, self.counter_, data, address);
            auto send_action = send_message(socket, self.shared_from_this());
            socket->register_send_action(std::move(send_action), common::get_shared_from_this(self));
        }

        auto send_continuous_message() -> asio::experimental::coro<std::size_t(std::optional<std::string_view>)>;
        auto check_response(std::span<char> response_msg) -> bool;

        // Setters:
        void set_socket(std::unique_ptr<asio::ip::udp::socket> socket) { socket_ = std::move(socket); }
        void set_remote_endpoint(asio::ip::udp::endpoint endpoint) { remote_endpoint_ = std::move(endpoint); }
        void set_timeout_seconds(int val) { timeout_seconds_ = val; }
        void set_write_response_msg(const std::vector<CommunicateEntryType>& msg, uint16_t address)
        {
            encode_write_msg(write_msg_response_buffer_, 0, msg, address);
        }

        void set_send_message(const RangedData auto& msg)
        {
            continuous_send_msg_ = std::span{ msg.begin(), msg.end() };
        }

        // Getters:
        [[nodiscard]] auto get_read_msg_buffer() const -> const auto& { return read_msg_buffer_; }
        [[nodiscard]] auto get_name() const -> const std::string& { return config_.name; }
        // [[nodiscard]] auto get_app() -> App& { return *app_; }
        auto get_socket() -> const udp::socket& { return *socket_; }
        auto get_socket_ptr() -> udp::socket* { return socket_.get(); }
        auto get_remote_endpoint() -> const udp::endpoint& { return remote_endpoint_; }
        [[nodiscard]] auto is_continuous() const -> bool { return is_continuous_; }
        auto get_response_msg() const -> std::string_view { return write_msg_response_buffer_.data(); }
        auto get_remote_ip_string() const { return remote_endpoint_.address().to_string(); }
        auto get_remote_port() const { return remote_endpoint_.port(); }

      protected:
        void close_socket();
        void set_continuous(bool is_continuous = true) { is_continuous_ = is_continuous; }
        void set_connection_bad() { is_connection_ok_ = false; }
        auto is_connection_ok() const -> bool { return is_connection_ok_; }

      private:
        bool is_continuous_ = false;
        bool is_connection_ok_ = true;
        std::atomic<bool> is_socket_closed_{ false };
        uint32_t counter_ = common::INIT_COUNT_VALUE;
        std::unique_ptr<udp::socket> socket_;
        udp::endpoint remote_endpoint_;
        process::SerializableMsgBuffer write_msg_buffer_;
        process::SerializableMsgBuffer write_msg_response_buffer_;
        std::span<const char> continuous_send_msg_;
        std::unique_ptr<asio::signal_set> signal_set_;
        std::vector<char> read_msg_buffer_;
        Config config_;

        int timeout_seconds_ = common::DEFAULT_TIMEOUT_SECONDS;

        static void encode_write_msg(process::SerializableMsgBuffer& buffer,
                                     uint32_t counter,
                                     const std::vector<CommunicateEntryType>& data,
                                     uint16_t address);
        static auto timer_countdown(auto* connection) -> asio::awaitable<void>;
        static auto send_message(std::shared_ptr<FecSwitchSocket> socket, std::shared_ptr<Base> connection)
            -> asio::awaitable<void>;
        void reset_read_msg_buffer() { std::ranges::fill(read_msg_buffer_, 0); }
    };

} // namespace srs::connection
