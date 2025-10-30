#include "srs/connections/ConnectionBase.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/FecSwitchSocket.hpp"
#include "srs/converters/SerializableBuffer.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/experimental/cancellation_condition.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>
// #include <cstddef>
#include <cstdint>
#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <memory>
// #include <optional>
#include <span>
#include <spdlog/spdlog.h>
// #include <string>
// #include <string_view>
#include <vector>

namespace srs::connection
{
    // Base::Base(const Info& info, std::string name, std::size_t buffer_size)
    //     : local_port_number_{ info.local_port_number }
    //     , name_{ std::move(name) }
    // // , app_{ info.control }
    // {
    //     spdlog::debug("Creating connection {} with buffer size: {}", name_, buffer_size);
    //     read_msg_buffer_.resize(buffer_size);
    // }

    Base::Base(const Config& config)
        : config_{ config }
    {

        spdlog::debug("Creating connection {} with buffer size: {}", config_.name, config.buffer_size);
        read_msg_buffer_.resize(config.buffer_size);
    }

    void Base::read_data_handle(std::span<BufferElementType> read_data)
    {
        spdlog::trace(
            "Connection {}: received {} bytes data: {:02x}", config_.name, read_data.size(), fmt::join(read_data, " "));
        if (write_msg_response_buffer_ == read_data)
        {
            spdlog::trace("Connection {}: received data is correct!", config_.name);
        }
        else
        {
            spdlog::trace("Connection {}: received data is incorrect! Supposed response should be: {:02x}",
                          config_.name,
                          fmt::join(write_msg_response_buffer_.data(), " "));
        }
    }

    // // TODO: remove this
    // auto Base::send_message(std::shared_ptr<Base> connection) -> asio::awaitable<void>
    // {
    //     spdlog::trace("Connection {}: Sending data ...", connection->get_name());
    //     auto data_size = co_await connection->socket_->async_send_to(
    //         asio::buffer(connection->write_msg_buffer_.data()), connection->remote_endpoint_, asio::use_awaitable);
    //     spdlog::trace("Connection {}: {} bytes data sent with {:02x}",
    //                   connection->get_name(),
    //                   data_size,
    //                   fmt::join(connection->write_msg_buffer_.data(), " "));
    // }

    auto Base::send_message(std::shared_ptr<FecSwitchSocket> socket, std::shared_ptr<Base> connection)
        -> asio::awaitable<void>
    {
        spdlog::debug("Connection {}: Sending data using external socket at time {} us...",
                      connection->get_name(),
                      socket->get_time_us());
        auto data_size = co_await socket->get_socket().async_send_to(
            asio::buffer(connection->write_msg_buffer_.data()), connection->remote_endpoint_, asio::use_awaitable);
        spdlog::debug("Connection {}: {} bytes data sent to the remote endpoint {} at time {} us: \n\t{:02x}",
                      connection->get_name(),
                      data_size,
                      connection->remote_endpoint_,
                      socket->get_time_us(),
                      fmt::join(connection->write_msg_buffer_.data(), " "));
        co_return;
    }

    // auto Base::send_continuous_message() -> asio::experimental::coro<std::size_t(std::optional<std::string_view>)>
    // {
    //     auto send_msg = std::string_view{};
    //     auto data_size = std::size_t{ 0 };
    //     while (true)
    //     {
    //         data_size = (not send_msg.empty()) ? socket_->send_to(asio::buffer(send_msg), remote_endpoint_) : 0;
    //         auto msg = co_yield data_size;

    //         if (not msg.has_value())
    //         {
    //             close();
    //             co_return;
    //         }
    //         else
    //         {
    //             send_msg = msg.value();
    //         }
    //     }
    // }

    // auto Base::new_shared_socket(int port_number) -> std::unique_ptr<udp::socket>
    // {
    //     auto socket = std::make_unique<udp::socket>(
    //         app_->get_io_context(), udp::endpoint{ udp::v4(), static_cast<asio::ip::port_type>(port_number) });
    //     spdlog::debug("Connection {}: Opening the socket from ip: {} with port: {}",
    //                   config_.name,
    //                   socket->local_endpoint().address().to_string(),
    //                   socket->local_endpoint().port());
    //     local_port_number_ = socket->local_endpoint().port();
    //     return socket;
    // }

    void Base::encode_write_msg(process::SerializableMsgBuffer& buffer,
                                uint32_t counter,
                                const std::vector<CommunicateEntryType>& data,
                                uint16_t address)
    {
        buffer.serialize(counter,
                         common::ZERO_UINT16_PADDING,
                         address,
                         common::WRITE_COMMAND_BITS,
                         common::DEFAULT_TYPE_BITS,
                         common::COMMAND_LENGTH_BITS);
        buffer.serialize(data);
    }

    auto Base::check_response(std::span<char> response_msg) -> bool
    {
        if (write_msg_response_buffer_.empty())
        {
            spdlog::trace("write_msg_response_buffer is empty!");
            return true;
        }
        return write_msg_response_buffer_ == response_msg;
    }

    void Base::close_socket()
    {
        if (not is_socket_closed_.load())
        {
            is_socket_closed_.store(true);
            spdlog::trace("Connection {}: Closing the socket ...", config_.name);
            // socket_->cancel();
            socket_->close();
            if (signal_set_ != nullptr)
            {
                spdlog::trace("Connection {}: cancelling signal ...", config_.name);
                signal_set_->cancel();
                spdlog::trace("Connection {}: signal is cancelled.", config_.name);
            }
            spdlog::trace("Connection {}: Socket is closed and cancelled.", config_.name);
        }
    }
} // namespace srs::connection
