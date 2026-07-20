#include "srs/connections/ConnectionBase.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/FecSwitchSocket.hpp"
#include "srs/converters/SerializableBuffer.hpp"
#include "srs/utils/AppReport.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/utils/ExitLogger.hpp"
#include <asio/awaitable.hpp>
#include <asio/buffer.hpp>
#include <asio/use_awaitable.hpp>
#include <cstdint>
#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <memory>
#include <span>
#include <spdlog/spdlog.h>

namespace srs::connection
{

    auto CommandBase::send_message(std::shared_ptr<FecCommandSocket> socket, std::shared_ptr<CommandBase> connection)
        -> asio::awaitable<void>
    {
        const auto _ = ExitLogger{};
        auto stat = AppReport::FecSwitchStat{};
        stat.send_time = socket->get_time_us();
        auto data_size = co_await socket->get_socket().async_send_to(
            asio::buffer(connection->write_msg_buffer_.data()), connection->remote_endpoint_, asio::use_awaitable);
        stat.receiv_time = socket->get_time_us();
        spdlog::debug("Connection {}: {} bytes data sent from local endpoint {} to the remote endpoint {}: \n\t{:02x}",
                      connection->get_name(),
                      data_size,
                      socket->get_socket().local_endpoint(),
                      connection->remote_endpoint_,
                      fmt::join(connection->write_msg_buffer_.data(), " "));
        stat.connection_name = connection->get_name();
        socket->log_time_stamps(fmt::format("{}", connection->remote_endpoint_), stat);
        co_return;
    }

    void CommandBase::encode_write_msg(process::SerializableMsgBuffer& buffer,
                                       uint32_t counter,
                                       std::span<const CommunicateEntryType> data,
                                       uint16_t address)
    {
        // TODO: make the end part a static data
        buffer.serialize(counter,
                         common::ZERO_UINT16_PADDING,
                         address,
                         common::WRITE_COMMAND_BITS,
                         common::DEFAULT_TYPE_BITS,
                         common::COMMAND_LENGTH_BITS);
        buffer.serialize(data);
    }

    auto CommandBase::check_response(std::span<char> response_msg) const -> bool
    {
        if (write_msg_response_buffer_.empty())
        {
            spdlog::trace("write_msg_response_buffer is empty!");
            return true;
        }
        return write_msg_response_buffer_ == response_msg;
    }
} // namespace srs::connection
