#include "SpecialSocketBase.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/UDPFormatters.hpp" // IWYU pragma: keep
#include <asio/as_tuple.hpp>
#include <asio/awaitable.hpp>
#include <asio/ip/basic_endpoint.hpp>
#include <asio/use_awaitable.hpp>
#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <system_error>

namespace srs::connection
{
    SpecialSocket::SpecialSocket(int port_number, io_context_type& io_context)
        : port_number_{ port_number }
        , cancel_timer_{ io_context }
    {
        socket_ = std::make_unique<udp::socket>(io_context);
        socket_->open(udp::v4());
        cancel_timer_.expires_at(std::chrono::system_clock::time_point::max());
    }

    auto SpecialSocket::cancel_coroutine() -> asio::awaitable<void>
    {
        // NOLINTBEGIN (clang-analyzer-core.NullDereference)
        [[maybe_unused]] auto err_code = co_await cancel_timer_.async_wait(asio::as_tuple(asio::use_awaitable));
        // NOLINTEND (clang-analyzer-core.NullDereference)
        spdlog::trace("Coroutine for the socket with local endpoint {} is cancelled.", socket_->local_endpoint());
    }

    auto SpecialSocket::bind_socket() -> std::error_code
    {
        auto local_endpoint = udp::endpoint{ udp::v4(), static_cast<asio::ip::port_type>(port_number_) };
        spdlog::trace("Binding a socket to the local endpoint {}.", local_endpoint);
        auto ec = std::error_code{};
        socket_->bind(local_endpoint, ec);
        return ec;
    }

    auto SpecialSocket::close_socket() -> std::error_code
    {
        auto err = std::error_code{};
        socket_->close(err);
        return err;
    }

    auto SpecialSocket::wait_for_listen_finish(std::chrono::seconds time) -> std::optional<std::future_status>
    {
        if (not listen_future_.valid())
        {
            return {};
        }
        return listen_future_.wait_for(time);
    }
} // namespace srs::connection
