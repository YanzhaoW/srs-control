#pragma once

#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/FecSwitchSocket.hpp"
#include "srs/converters/SerializableBuffer.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonConcepts.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/utils/UDPFormatters.hpp" // IWYU pragma: keep
#include <algorithm>
#include <atomic>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>
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
        auto check_response(std::span<char> response_msg) -> bool;

        // Setters:
        void set_remote_endpoint(asio::ip::udp::endpoint endpoint) { remote_endpoint_ = std::move(endpoint); }
        void set_write_response_msg(const std::vector<CommunicateEntryType>& msg, uint16_t address)
        {
            encode_write_msg(write_msg_response_buffer_, 0, msg, address);
        }

        // Getters:
        [[nodiscard]] auto get_name() const -> const std::string& { return config_.name; }
        auto get_remote_endpoint() -> const udp::endpoint& { return remote_endpoint_; }
        auto get_response_msg() const -> std::string_view { return write_msg_response_buffer_.data(); }
        auto get_remote_ip_string() const { return remote_endpoint_.address().to_string(); }
        auto get_remote_port() const { return remote_endpoint_.port(); }

      private:
        uint32_t counter_ = common::INIT_COUNT_VALUE;
        udp::endpoint remote_endpoint_;
        process::SerializableMsgBuffer write_msg_buffer_;
        process::SerializableMsgBuffer write_msg_response_buffer_;
        Config config_;

        static void encode_write_msg(process::SerializableMsgBuffer& buffer,
                                     uint32_t counter,
                                     const std::vector<CommunicateEntryType>& data,
                                     uint16_t address);
        static auto timer_countdown(auto* connection) -> asio::awaitable<void>;
        static auto send_message(std::shared_ptr<FecSwitchSocket> socket, std::shared_ptr<Base> connection)
            -> asio::awaitable<void>;
    };

} // namespace srs::connection
