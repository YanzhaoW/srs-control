#pragma once

#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/SpecialSocketBase.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/impl/co_spawn.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/system_timer.hpp>
#include <chrono>
#include <fmt/ranges.h>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <spdlog/spdlog.h>
#include <vector>

namespace srs::connection
{
    template <int buffer_size>
    class Base;

    class FecSwitchSocket : public SpecialSocket
    {
      public:
        using SmallConnection = Base<common::SMALL_READ_MSG_BUFFER_SIZE>;
        using ConnectionType = SmallConnection;

        // getters:
        auto get_strand() -> auto& { return strand_; }
        auto get_time_us() const -> auto
        {
            return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time_)
                .count();
        }
        void launch_actions();

      private:
        friend SpecialSocket;
        using SmallConnections = std::vector<std::shared_ptr<SmallConnection>>;
        asio::strand<io_context_type::executor_type> strand_;
        std::mutex mut_;
        std::map<UDPEndpoint, SmallConnections> all_connections_;
        ReadBufferType<common::SMALL_READ_MSG_BUFFER_SIZE> read_msg_buffer_{};
        std::chrono::time_point<std::chrono::steady_clock> start_time_ = std::chrono::steady_clock::now();

        using ActionType = decltype(asio::co_spawn(strand_, asio::awaitable<void>(), asio::deferred));
        std::vector<ActionType> action_queue_;

        void register_send_action_imp(asio::awaitable<void> action, std::shared_ptr<SmallConnection> connection);
        auto get_response_msg_buffer() -> auto& { return read_msg_buffer_; }
        auto is_finished() -> bool;
        void print_error() const;
        void response_handler(const UDPEndpoint& endpoint, std::span<char> response);

        explicit FecSwitchSocket(int port_number, io_context_type& io_context);
        void deregister_connection(const UDPEndpoint& endpoint,
                                   std::span<char> response,
                                   SmallConnections& connections);
        void print_available_responses() const;
    };

} // namespace srs::connection
