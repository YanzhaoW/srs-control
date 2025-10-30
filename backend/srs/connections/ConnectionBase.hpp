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
        // explicit Base(const Info& info, std::string name, std::size_t buffer_size =
        // common::SMALL_READ_MSG_BUFFER_SIZE);
        explicit Base(const Config& config);

        // possible overload from derived class
        void read_data_handle(std::span<BufferElementType> read_data);

        void close() { close_socket(); }
        static void on_fail() { spdlog::debug("default on_fail is called!"); }

        // void listen(this auto&& self, bool is_non_stop = false)
        // {
        //     using asio::experimental::awaitable_operators::operator||;
        //     if (self.socket_ == nullptr)
        //     {
        //         self.socket_ = self.new_shared_socket(self.local_port_number_);
        //     }

        //     co_spawn(self.app_->get_io_context(),
        //              signal_handling(common::get_shared_from_this(self)) ||
        //                  listen_message(common::get_shared_from_this(self), is_non_stop),
        //              asio::detached);
        //     spdlog::trace("Connection {}: spawned listen coroutine", self.config_.name);
        // }

        // void communicate(this auto&& self, const std::vector<CommunicateEntryType>& data, uint16_t address)
        // {
        //     using asio::experimental::awaitable_operators::operator||;
        //     if (self.socket_ == nullptr)
        //     {
        //         self.socket_ = self.new_shared_socket(self.local_port_number_);
        //     }

        //     auto listen_action = co_spawn(self.app_->get_io_context(),
        //                                   signal_handling(common::get_shared_from_this(self)) ||
        //                                       listen_message(common::get_shared_from_this(self), false),
        //                                   asio::deferred);

        //     self.encode_write_msg(self.write_msg_buffer_, self.counter_, data, address);
        //     auto send_action =
        //         co_spawn(self.app_->get_io_context(), send_message(self.shared_from_this()), asio::deferred);
        //     auto group = asio::experimental::make_parallel_group(std::move(listen_action), std::move(send_action));
        //     auto fut = group.async_wait(asio::experimental::wait_for_all(), asio::use_future);
        //     spdlog::trace("Connection {}: start communication", self.config_.name);
        //     fut.get();
        // }

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
        // int local_port_number_ = 0;
        std::atomic<bool> is_socket_closed_{ false };
        uint32_t counter_ = common::INIT_COUNT_VALUE;
        // std::string name_ = "ConnectionBase";
        // gsl::not_null<App*> app_;
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
        // static auto signal_handling(SharedConnectionPtr auto connection) -> asio::awaitable<void>;
        static auto timer_countdown(auto* connection) -> asio::awaitable<void>;
        // static auto listen_message(SharedConnectionPtr auto connection, bool is_non_stop = false)
        //     -> asio::awaitable<void>;
        // static auto send_message(std::shared_ptr<Base> connection) -> asio::awaitable<void>;
        static auto send_message(std::shared_ptr<FecSwitchSocket> socket, std::shared_ptr<Base> connection)
            -> asio::awaitable<void>;
        void reset_read_msg_buffer() { std::ranges::fill(read_msg_buffer_, 0); }
    };

    // auto Base::signal_handling(SharedConnectionPtr auto connection) -> asio::awaitable<void>
    // {
    //     connection->signal_set_ = std::make_unique<asio::signal_set>(co_await asio::this_coro::executor, SIGINT);
    //     spdlog::trace("Connection {}: waiting for signals", connection->get_name());
    //     auto [error, sig_num] = co_await connection->signal_set_->async_wait(asio::as_tuple(asio::use_awaitable));
    //     if (error == asio::error::operation_aborted)
    //     {
    //         spdlog::trace("Connection {}: Signal ended with {}", connection->get_name(), error.message());
    //     }
    //     else
    //     {
    //         fmt::print("\n");
    //         spdlog::trace(
    //             "Connection {}: Signal ID {} is called with {:?}!", connection->get_name(), sig_num,
    //             error.message());
    //         connection->close();
    //     }
    // }

    // auto Base::listen_message(SharedConnectionPtr auto connection, bool is_non_stop) -> asio::awaitable<void>
    // {
    //     using asio::experimental::awaitable_operators::operator||;

    //     spdlog::debug("Connection {}: starting to listen ...", connection->get_name());

    //     auto io_context = co_await asio::this_coro::executor;
    //     auto timer = asio::steady_timer{ io_context };
    //     if (is_non_stop)
    //     {
    //         timer.expires_at(decltype(timer)::time_point::max());
    //     }
    //     else
    //     {
    //         timer.expires_after(std::chrono::seconds{ connection->timeout_seconds_ });
    //     }

    //     while (true)
    //     {
    //         if (not connection->socket_->is_open() or connection->is_socket_closed_.load())
    //         {
    //             co_return;
    //         }

    //         auto receive_data_size = co_await (
    //             connection->socket_->async_receive(asio::buffer(connection->read_msg_buffer_), asio::use_awaitable)
    //             || timer.async_wait(asio::use_awaitable));
    //         auto read_msg = std::span{ connection->read_msg_buffer_.data(), std::get<std::size_t>(receive_data_size)
    //         }; connection->read_data_handle(read_msg); if (not is_non_stop and
    //         std::holds_alternative<std::monostate>(receive_data_size))
    //         {
    //             if (not connection->is_continuous())
    //             {
    //                 spdlog::error("Connection {}: Message listening TIMEOUT after {} seconds.",
    //                               connection->get_name(),
    //                               connection->timeout_seconds_);
    //                 connection->on_fail();
    //             }
    //             else
    //             {
    //                 spdlog::info("Connection {}: Message listening TIMEOUT after {} seconds.",
    //                              connection->get_name(),
    //                              connection->timeout_seconds_);
    //             }
    //             break;
    //         }

    //         connection->reset_read_msg_buffer();
    //         if (not connection->is_continuous() or connection->get_app().get_status().is_on_exit.load())
    //         {
    //             break;
    //         }
    //     }
    //     connection->close();
    // }
} // namespace srs::connection
