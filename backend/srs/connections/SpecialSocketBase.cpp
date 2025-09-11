#include "SpecialSocketBase.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/utils/CommonAlias.hpp"
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/system/detail/error_code.hpp>
#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>

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
        [[maybe_unused]] auto err_code = co_await cancel_timer_.async_wait(asio::as_tuple(asio::use_awaitable));
    }

    void SpecialSocket::bind_socket()
    {
        [[maybe_unused]] auto error_code =
            socket_->bind(udp::endpoint{ udp::v4(), static_cast<asio::ip::port_type>(port_number_) }, socket_ec_);
    }
    void SpecialSocket::close_socket() { socket_->close(); }

    auto SpecialSocket::wait_for_listen_finish(std::chrono::seconds time) -> std::optional<std::future_status>
    {
        if (not listen_future_.valid())
        {
            return {};
        }
        return listen_future_.wait_for(time);
    }
} // namespace srs::connection
