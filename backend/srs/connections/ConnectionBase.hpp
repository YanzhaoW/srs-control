#pragma once

#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/FecSwitchSocket.hpp"
#include "srs/converters/SerializableBuffer.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/utils/UDPFormatters.hpp" // IWYU pragma: keep
#include <algorithm>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/udp.hpp>
#include <cstddef>
#include <cstdint>
#include <fmt/base.h>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <span>
#include <spdlog/spdlog.h>
#include <string_view>
#include <utility>
#include <vector>

namespace srs::connection
{
    class FecCommandSocket;

    class CommandBase : public std::enable_shared_from_this<CommandBase>
    {
      public:
        explicit CommandBase(const std::string_view name)
            : name_{ name }
        {
        }

        void send_message_from(this auto&& self, std::shared_ptr<FecCommandSocket> socket)
        {
            self.communicate(self.get_suffix(), common::NULL_ADDRESS, std::move(socket));
        }

        [[nodiscard]] auto check_response(std::span<char> response_msg) const -> bool;

        // Setters:
        void set_remote_endpoint(asio::ip::udp::endpoint endpoint) { remote_endpoint_ = std::move(endpoint); }

        // Getters:
        [[nodiscard]] auto get_name() const -> std::string_view { return name_; }
        auto get_remote_endpoint() -> const udp::endpoint& { return remote_endpoint_; }
        [[nodiscard]] auto get_response_msg() const -> std::string_view { return write_msg_response_buffer_.data(); }
        [[nodiscard]] auto get_remote_ip_string() const { return remote_endpoint_.address().to_string(); }
        [[nodiscard]] auto get_remote_port() const { return remote_endpoint_.port(); }

      protected:
        void set_write_response_msg(const std::vector<CommunicateEntryType>& msg, uint16_t address)
        {
            encode_write_msg(write_msg_response_buffer_, 0, msg, address);
        }

      private:
        uint32_t counter_ = common::INIT_COUNT_VALUE;
        udp::endpoint remote_endpoint_;
        process::SerializableMsgBuffer write_msg_buffer_;
        process::SerializableMsgBuffer write_msg_response_buffer_;
        std::string_view name_;

        static void encode_write_msg(process::SerializableMsgBuffer& buffer,
                                     uint32_t counter,
                                     std::span<const CommunicateEntryType> data,
                                     uint16_t address);

        static auto timer_countdown(auto* connection) -> asio::awaitable<void>;

        static auto send_message(std::shared_ptr<FecCommandSocket> socket, std::shared_ptr<CommandBase> connection)
            -> asio::awaitable<void>;

        void communicate(this auto&& self,
                         std::span<const CommunicateEntryType> data,
                         uint16_t address,
                         std::shared_ptr<FecCommandSocket> socket)
        {
            self.encode_write_msg(self.write_msg_buffer_, self.counter_, data, address);
            auto send_action = send_message(socket, self.shared_from_this());
            socket->register_send_action(std::move(send_action), common::get_shared_from_this(self));
        }
    };
} // namespace srs::connection
