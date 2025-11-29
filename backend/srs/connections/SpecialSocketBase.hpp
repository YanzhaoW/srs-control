#pragma once

#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <concepts>
#include <expected>
#include <fmt/ranges.h>
#include <future>
#include <memory>
#include <optional>
#include <span>
#include <spdlog/spdlog.h>
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

            asio::buffer(socket.get_response_msg_buffer());

            requires IsConnectionType<typename SocketType::ConnectionType>::value;

            { socket.response_handler(UDPEndpoint{}, std::span<char>{}) } -> std::same_as<void>;
            { socket.is_finished() } -> std::same_as<bool>;
            { socket.print_error() } -> std::same_as<void>;
            { socket.register_send_action_imp(asio::awaitable<void>{}, connection) } -> std::same_as<void>;
        };

    class SpecialSocket : public std::enable_shared_from_this<SpecialSocket>
    {
      public:
        template <SpecialSocketDerived SocketType, typename... Args>
        static auto create(int port_number, io_context_type& io_context, Args... args)
            -> std::expected<std::shared_ptr<SocketType>, boost::system::error_code>;

        void register_send_action(
            this auto&& self,
            asio::awaitable<void> action,
            std::shared_ptr<typename std::remove_cvref_t<decltype(self)>::ConnectionType> connection);

        void listen(this auto& self, io_context_type& io_context);

        auto is_valid() const -> bool { return is_valid_; }

        auto cancel_listen_after(this auto&& self,
                                 io_context_type& io_context,
                                 std::chrono::seconds waiting_time = std::chrono::seconds(2)) -> std::future<void>;

        auto wait_for_listen_finish(std::chrono::seconds time) -> std::optional<std::future_status>;

        // getters:
        [[nodiscard]] auto get_socket() const -> auto& { return *socket_; }
        [[nodiscard]] auto get_port() const { return port_number_; }
        [[nodiscard]] auto get_socket_error_code() const -> auto { return socket_ec_; }
        auto get_future() -> auto& { return listen_future_; }

      protected:
        explicit SpecialSocket(int port_number, io_context_type& io_context);
        auto get_cancel_timer() -> auto& { return cancel_timer_; };

      private:
        bool is_valid_ = false;
        int port_number_ = 0;
        std::unique_ptr<udp::socket> socket_;
        std::shared_future<std::variant<std::monostate, std::monostate>> listen_future_;
        boost::system::error_code socket_ec_;
        asio::system_timer cancel_timer_; //!< Used for cancel the unfinished coroutine

        auto cancel_coroutine() -> asio::awaitable<void>;
        void bind_socket();
        void close_socket();

        // NOTE: Coroutine should always be static to avoid lifetime issue.
        template <SpecialSocketDerived SocketType>
        static auto listen_all_connections(std::shared_ptr<SocketType> socket) -> asio::awaitable<void>;
    };

    template <SpecialSocketDerived SocketType, typename... Args>
    auto SpecialSocket::create(int port_number, io_context_type& io_context, Args... args)
        -> std::expected<std::shared_ptr<SocketType>, boost::system::error_code>
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
    auto SpecialSocket::listen_all_connections(std::shared_ptr<SocketType> socket) -> asio::awaitable<void>
    {
        spdlog::debug("Local socket with port {} starts to listen ...", socket->get_port());
        socket->bind_socket();
        auto remote_endpoint = udp::endpoint{};
        while (true)
        {
            const auto [err_code, receive_size] = co_await socket->get_socket().async_receive_from(
                asio::buffer(socket->get_response_msg_buffer()), remote_endpoint, asio::as_tuple(asio ::use_awaitable));
            if (not err_code)
            {
                auto read_msg = std::span{ socket->get_response_msg_buffer().data(), receive_size };
                [[maybe_unused]] auto remote_ec = boost::system::error_code{};
                socket->response_handler(remote_endpoint, read_msg);
            }
        }
        // TODO: close the socket here
        spdlog::trace("Coroutine for the local socket with port {} has existed.", socket->get_port());
        co_return;
    }

    void SpecialSocket::register_send_action(
        this auto&& self,
        asio::awaitable<void> action,
        std::shared_ptr<typename std::remove_cvref_t<decltype(self)>::ConnectionType> connection)
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
        using boost::asio::experimental::awaitable_operators::operator||;
        self.listen_future_ =
            asio::co_spawn(io_context,
                           listen_all_connections(common::get_shared_from_this(self)) || self.cancel_coroutine(),
                           asio::use_future)
                .share();
    }

    auto SpecialSocket::cancel_listen_after(this auto&& self,
                                            io_context_type& io_context,
                                            std::chrono::seconds waiting_time) -> std::future<void>
    {
        auto waiting_action = [](auto socket, std::chrono::seconds waiting_time) -> asio::awaitable<void>
        {
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
