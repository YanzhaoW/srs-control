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
#include <cstdint>
#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <memory>
#include <span>
#include <spdlog/spdlog.h>
#include <vector>

namespace srs::connection
{

    auto Base::send_message(std::shared_ptr<FecCommandSocket> socket, std::shared_ptr<Base> connection)
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
} // namespace srs::connection
