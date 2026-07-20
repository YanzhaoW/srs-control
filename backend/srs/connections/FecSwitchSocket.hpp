#pragma once

#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/SpecialSocketBase.hpp"
#include "srs/utils/AppReport.hpp"
#include "srs/utils/CommonAlias.hpp"
#include <asio/awaitable.hpp>
#include <asio/deferred.hpp>
#include <asio/impl/co_spawn.hpp>
#include <asio/strand.hpp>
#include <asio/system_timer.hpp>
#include <chrono>
#include <cstddef>
#include <fmt/ranges.h>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>
#include <vector>

namespace srs::connection
{
    class CommandBase;

    class FecCommandSocket : public SpecialSocket
    {
      public:
        using SmallConnection = CommandBase;
        using ConnectionType = SmallConnection;
        using US = std::chrono::microseconds;
        using USCount = decltype(US{}.count());
        using StartEndTime = std::pair<USCount, USCount>;

        // getters:
        auto get_strand() -> auto& { return strand_; }
        auto get_time_us() const -> USCount
        {
            return std::chrono::duration_cast<US>(std::chrono::steady_clock::now() - start_time_).count();
        }
        void launch_send_actions();

        void log_time_stamps(std::string remote_port, AppReport::FecSwitchStat stat)
        {
            time_stamps_.emplace_back(std::move(remote_port), stat);
        }

        void before_socket_close() { register_report(); }

      private:
        friend SpecialSocket;
        using SmallConnections = std::vector<std::shared_ptr<SmallConnection>>;
        asio::strand<io_context_type::executor_type> strand_;
        std::mutex mut_;
        std::map<UDPEndpoint, SmallConnections> all_connections_;
        std::vector<char> read_msg_buffer_;
        std::chrono::time_point<std::chrono::steady_clock> start_time_ = std::chrono::steady_clock::now();
        std::vector<std::pair<std::string, AppReport::FecSwitchStat>> time_stamps_;

        using ActionType = decltype(asio::co_spawn(strand_, asio::awaitable<void>(), asio::deferred));
        std::vector<ActionType> action_queue_;

        // constructor
        explicit FecCommandSocket(int port_number, io_context_type& io_context);

        void register_send_action_imp(asio::awaitable<void> action, const std::shared_ptr<SmallConnection>& connection);
        auto get_response_msg_buffer() -> std::span<char> { return std::span{ read_msg_buffer_ }; }
        auto is_finished() -> bool;
        void print_error() const;
        void response_handler(const UDPEndpoint& endpoint, std::size_t read_size);

        void deregister_connection(const UDPEndpoint& endpoint,
                                   std::span<char> response,
                                   SmallConnections& connections);
        void print_available_responses() const;

        void register_report();
    };

} // namespace srs::connection
