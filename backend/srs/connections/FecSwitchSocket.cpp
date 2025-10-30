#include "FecSwitchSocket.hpp"
#include "srs/connections/ConnectionBase.hpp" // IWYU pragma: keep
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/connections/SpecialSocketBase.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/utils/UDPFormatters.hpp" // IWYU pragma: keep
#include <algorithm>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/experimental/cancellation_condition.hpp>
#include <boost/asio/experimental/parallel_group.hpp>
#include <boost/asio/impl/co_spawn.hpp>
#include <boost/asio/ip/basic_endpoint.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/thread/future.hpp>
#include <fmt/ranges.h>
#include <memory>
#include <mutex>
#include <ranges>
#include <span>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>

namespace srs::connection
{
    namespace
    {
        auto remove_once_if(auto& connections, auto unitary_opt) -> bool
        {
            auto iter = std::ranges::find_if(
                connections, [unitary_opt](const auto& connection) -> bool { return not unitary_opt(connection); });
            if (iter == connections.end())
            {
                return false;
            }
            std::iter_swap(iter, connections.end() - 1);
            connections.pop_back();
            return true;
        }

        auto print_connections(const auto& connections) -> std::string
        {
            auto join_connection = [](const auto& connection)
            { return fmt::format("{:02x}", fmt::join(connection->get_response_msg(), " ")); };
            return fmt::format("{}", fmt::join(connections | std::views::transform(join_connection), "\n\t"));
        }
    } // namespace

    FecSwitchSocket::FecSwitchSocket(int port_number, io_context_type& io_context)
        : SpecialSocket{ port_number, io_context }
        , strand_{ asio::make_strand(io_context.get_executor()) }

    {
        read_msg_buffer_.resize(common::SMALL_READ_MSG_BUFFER_SIZE);
    }

    void FecSwitchSocket::register_send_action_imp(asio::awaitable<void> action,
                                                   std::shared_ptr<SmallConnection> connection)
    {
        auto time = get_time_us();
        spdlog::debug(
            "Registering data sent to the remote endpoint {} at time {} us", connection->get_remote_endpoint(), time);
        // asio::co_spawn(strand_, std::move(action), asio::detached);
        auto co_action = asio::co_spawn(strand_, std::move(action), asio::deferred);
        action_queue_.push_back(std::move(co_action));

        auto lock = std::lock_guard{ mut_ };
        const auto& remote_endpoint = connection->get_remote_endpoint();
        auto& connections = all_connections_.try_emplace(remote_endpoint, SmallConnections{}).first->second;
        connections.push_back(std::move(connection));
    }

    void FecSwitchSocket::launch_actions()
    {
        asio::experimental::make_parallel_group(std::move(action_queue_))
            .async_wait(asio::experimental::wait_for_all(), asio::use_future)
            .get();
    }

    void FecSwitchSocket::deregister_connection(const UDPEndpoint& endpoint,
                                                std::span<char> response,
                                                SmallConnections& connections)
    {
        if (connections.empty())
        {
            spdlog::debug("Response from the remote endpoint {} to the local port {} is received. But no further "
                          "message from the endpoint is not required anymore!",
                          endpoint,
                          get_port());
            return;
        }
        auto res = remove_once_if(connections,
                                  [response](const std::shared_ptr<SmallConnection>& connection) -> bool
                                  {
                                      auto is_same = connection->check_response(response);
                                      return not is_same;
                                  });
        if (res)
        {
            spdlog::debug("Response from the remote endpoint {} is recognized by the local socket with port {}",
                          endpoint,
                          get_port());
            // print_available_responses();
        }
        else
        {
            spdlog::warn("From remote endpoint: {}, local socket with port {} received an unrecognized msg: {:02x}",
                         endpoint,
                         get_port(),
                         fmt::join(response, " "));
            print_available_responses();
        }
    }

    void FecSwitchSocket::print_available_responses() const
    {
        spdlog::info(
            "Available responses are:\n\t{}",
            fmt::join(all_connections_ | std::views::values |
                          std::views::filter([](const auto& connections) -> bool { return not connections.empty(); }) |
                          std::views::transform([](const auto& connections) { return print_connections(connections); }),
                      "\n\t"));
    }

    void FecSwitchSocket::print_error() const
    {
        spdlog::error(
            "TIMEOUT during local port {} waiting for the responses from the following ip/port:\n\t{}",
            get_port(),
            fmt::join(
                all_connections_ |
                    std::views::filter([](const auto& key_value) -> bool { return not key_value.second.empty(); }) |
                    std::views::transform(
                        [](const auto& key_value)
                        { return fmt::format("{}: {}", key_value.first, print_connections(key_value.second)); }),
                "\n\t"));
    }

    void FecSwitchSocket::response_handler(const UDPEndpoint& endpoint, std::span<char> response)
    {
        auto lock = std::lock_guard{ mut_ };
        spdlog::trace("Local port {} received a response from a remote endpoint {}: \n\t{:02x}",
                      get_port(),
                      endpoint,
                      fmt::join(response, " "));
        auto connections_iter = all_connections_.find(endpoint);
        if (connections_iter == all_connections_.end())
        {
            spdlog::error("Local socket with port {} received an unknown remote endpoint: {}", get_port(), endpoint);
            return;
        }
        deregister_connection(endpoint, response, connections_iter->second);
    }

    auto FecSwitchSocket::is_finished() -> bool
    {

        return std::ranges::all_of(all_connections_ | std::views::values,
                                   [](const auto& connections) -> bool { return connections.empty(); });
    }
} // namespace srs::connection
