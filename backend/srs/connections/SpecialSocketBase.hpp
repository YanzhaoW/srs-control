#pragma once

#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include "srs/utils/ExitLogger.hpp"
#include <asio/as_tuple.hpp>
#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/system_timer.hpp>
#include <asio/this_coro.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/use_future.hpp>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <fmt/ranges.h>
#include <future>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

namespace srs::connection
{
    class SpecialSocket;

    template <typename SocketType>
    concept SpecialSocketDerived =
        requires(SocketType socket, std::shared_ptr<typename SocketType::ConnectionType> connection) {
            requires std::derived_from<SocketType, SpecialSocket>;
            requires not std::is_same_v<SocketType, SpecialSocket>;

            asio::mutable_buffer{ socket.get_response_msg_buffer() };

            requires IsConnectionType<typename SocketType::ConnectionType>::value;

            { socket.response_handler(UDPEndpoint{}, std::size_t{}) } -> std::same_as<void>;
            { socket.is_finished() } -> std::same_as<bool>;
            { socket.print_error() } -> std::same_as<void>;
            { socket.register_send_action_imp(asio::awaitable<void>{}, connection) } -> std::same_as<void>;
        };

    /**
     * @brief Base socket class for sending and listening messages.
     */
    class SpecialSocket : public std::enable_shared_from_this<SpecialSocket>
    {
      public:
        template <SpecialSocketDerived SocketType, typename... Args>
        static auto create(int port_number, io_context_type& io_context, Args&&... args)
            -> std::expected<std::shared_ptr<SocketType>, std::error_code>;

        /**
         * @brief Registering the send action.
         *
         * Invoke an action corresponding the type of connection specified in the socket class. The connection type
         * should all derive from srs::connection::ConnectionBase class.
         * @param self Deduce this self reference
         * @param action An asio::awaitable action object.
         * @param connection A connection shared pointer.
         */
        void register_send_action(
            this auto&& self,
            asio::awaitable<void> action,
            const std::shared_ptr<typename std::remove_cvref_t<decltype(self)>::ConnectionType>& connection);

        /**
         * @brief Listen to any UDP data
         *
         * @param self Deduce this self reference
         * @param io_context Reference to the io_context
         */
        void listen(this auto& self, io_context_type& io_context);

        /**
         * @brief Cancelling the listening action after a certain time
         *
         * @param self Deduce this self reference
         * @param io_context Reference to the io_context
         * @param waiting_time Time until the listen action is cancelled.
         * @return a future object to check whether the listen action is cancelled.
         */
        auto cancel_listen_after(this auto&& self,
                                 io_context_type& io_context,
                                 std::chrono::seconds waiting_time = std::chrono::seconds(2)) -> std::future<void>;

        /**
         * @brief Blocking the current thread by waiting the listen action to finish until a certain time duration.
         *
         * @param time Time to wait for the listen action to finish.
         * @return The result of waiting.
         */
        auto wait_for_listen_finish(std::chrono::seconds time) -> std::optional<std::future_status>;

        // getters:

        /**
         * @brief Getter for the asio udp socket object.
         *
         * @return shared pointer to the udp socket.
         */
        [[nodiscard]] auto get_socket() const -> udp::socket& { return *socket_; }

        /**
         * @brief Getter for the port number.
         *
         * @return Port number.
         */
        [[nodiscard]] auto get_port() const -> int { return port_number_; }

        /**
         * @brief Getter the possible error during the socket binding.
         *
         * @return Error code from the binding.
         */
        [[nodiscard]] auto get_socket_error_code() const -> std::error_code { return socket_ec_; }

        /**
         * @brief Getter for the status of the listen action.
         *
         * @return The future status of the listen action coroutine.
         */
        auto get_future() -> auto& { return listen_future_; }

        /**
         * @brief Getter for the number of frames read.
         *
         * @return Number of frames read.
         */
        auto get_n_records() const -> std::size_t { return n_records_; }

        /**
         * @brief Getter for the number of bytes read.
         *
         * @return Number of bytes read.
         */
        auto get_n_bytes() const -> std::size_t { return n_bytes; }

        /**
         * @brief Getter for the total listening time in nano seconds.
         *
         * @return Total listening time.
         */
        auto get_total_time_ns() const -> std::uint64_t { return total_time_ns_; }

      protected:
        /**
         * @brief Projected constructor, which can only be called from the derived classes.
         *
         * @param port_numer Port number binding to the socket for the listening action.
         * @param io_context asio io context.
         */
        explicit SpecialSocket(int port_number, io_context_type& io_context);
        auto get_cancel_timer() -> auto& { return cancel_timer_; };

      private:
        int port_number_ = 0;
        std::unique_ptr<udp::socket> socket_;
        std::shared_future<std::variant<std::monostate, std::monostate>> listen_future_;
        std::error_code socket_ec_;
        asio::system_timer cancel_timer_; //!< Used for cancel the unfinished coroutine

        // for the time measurement
        [[maybe_unused]] std::chrono::steady_clock clock_;
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::nanoseconds> time_point_;
        std::size_t n_records_ = 0;
        std::size_t n_bytes = 0;
        std::uint64_t total_time_ns_ = 0;

        auto cancel_coroutine() -> asio::awaitable<void>;
        void bind_socket();
        void close_socket();

        // NOTE: Coroutine should always be static to avoid lifetime issue.
        template <SpecialSocketDerived SocketType>
        static auto async_listen_all_connections(std::shared_ptr<SocketType> socket) -> asio::awaitable<void>;
    };

    template <SpecialSocketDerived SocketType, typename... Args>
    auto SpecialSocket::create(int port_number, io_context_type& io_context, Args&&... args)
        -> std::expected<std::shared_ptr<SocketType>, std::error_code>
    {
        auto socket =
            std::shared_ptr<SocketType>(new SocketType{ port_number, io_context, std::forward<Args>(args)... });
        auto error_code = socket->get_socket_error_code();
        if (error_code)
        {
            spdlog::critical("Local socket failed to bind to the port {} due to the error: {}",
                             socket->get_port(),
                             error_code.message());
            return std::unexpected{ error_code };
        }
        socket->listen(io_context);
        return socket;
    }

    template <SpecialSocketDerived SocketType>
    auto SpecialSocket::async_listen_all_connections(std::shared_ptr<SocketType> socket) -> asio::awaitable<void>
    {
        const auto port_str = fmt::format("{}", socket->get_port());
        const auto _ = ExitLogger{ port_str };
        socket->bind_socket();
        auto remote_endpoint = udp::endpoint{};
        spdlog::debug("Local socket with port {} starts to listen ...", socket->get_port());
        while (true)
        {
            const auto [err_code, receive_size] = co_await socket->get_socket().async_receive_from(
                asio::mutable_buffer{ socket->get_response_msg_buffer() },
                remote_endpoint,
                asio::as_tuple(asio::use_awaitable));

            socket->time_point_ = socket->clock_.now();

            if (not err_code)
            {
                socket->response_handler(remote_endpoint, receive_size);
            }

            ++socket->n_records_;
            socket->n_bytes += receive_size;
            socket->total_time_ns_ += static_cast<uint64_t>((socket->clock_.now() - socket->time_point_).count());
        }
        // TODO: close the socket here
        spdlog::trace("Coroutine for the local socket with port {} has existed.", socket->get_port());
        co_return;
    }

    void SpecialSocket::register_send_action(
        this auto&& self,
        asio::awaitable<void> action,
        const std::shared_ptr<typename std::remove_cvref_t<decltype(self)>::ConnectionType>& connection)
    {
        spdlog::trace("Registering send action from connection {} with remote endpoint {}.",
                      connection->get_name(),
                      connection->get_remote_endpoint());
        self.register_send_action_imp(std::move(action), connection);
        spdlog::trace("Send action to the remote endpoint {} has been registered.",
                      connection->get_name(),
                      connection->get_remote_endpoint());
    }

    void SpecialSocket::listen(this auto& self, io_context_type& io_context)
    {
        using asio::experimental::awaitable_operators::operator||;
        self.listen_future_ =
            asio::co_spawn(io_context,
                           async_listen_all_connections(common::get_shared_from_this(self)) || self.cancel_coroutine(),
                           asio::use_future)
                .share();
    }

    auto SpecialSocket::cancel_listen_after(this auto&& self,
                                            io_context_type& io_context,
                                            std::chrono::seconds waiting_time) -> std::future<void>
    {
        auto waiting_action = [](auto socket, std::chrono::seconds waiting_time) -> asio::awaitable<void>
        {
            const auto _ = ExitLogger{ "waiting_action" };
            spdlog::trace("Waiting local socket with port {} to finish listening ...", socket->get_port());
            if (socket->is_finished())
            {
                spdlog::trace("Local socket with port {} finished listening.", socket->get_port());
                co_return;
            }
            auto timer = asio::system_timer{ co_await asio::this_coro::executor };
            timer.expires_after(waiting_time);
            [[maybe_unused]] auto err_code = co_await timer.async_wait(asio::as_tuple(asio::use_awaitable));
            socket->cancel_timer_.cancel();
            if (not socket->is_finished())
            {
                socket->print_error();
            }
        };
        return asio::co_spawn(
            io_context, waiting_action(common::get_shared_from_this(self), waiting_time), asio::use_future);
    }
} // namespace srs::connection
