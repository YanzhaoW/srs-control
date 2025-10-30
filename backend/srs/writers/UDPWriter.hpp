#pragma once

#include "srs/Application.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonDefinitions.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/uses_executor.hpp>
#include <boost/cobalt/generator.hpp>
#include <boost/cobalt/this_coro.hpp>
#include <boost/cobalt/this_thread.hpp>
#include <boost/thread/future.hpp>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <srs/connections/ConnectionBase.hpp>
#include <srs/workflow/TaskDiagram.hpp>
#include <srs/writers/DataWriterOptions.hpp>
#include <string_view>
#include <utility>

namespace srs::connection
{
    class UDPWriterConnection : public Base
    {
      public:
        explicit UDPWriterConnection(const Config& config, io_executor_type io_executor)
            // : Base(info, "UDP writer")
            : Base(config)
            , io_executor{ std::move(io_executor) }
        {
        }
        auto get_executor() const { return io_executor; }
        using OutputType = std::size_t;
        using InputType = std::string_view;

        // auto send_continuous_message(asio::executor_arg_t /*unused*/, cobalt::executor /*unused*/)
        //     -> cobalt::generator<std::size_t, std::optional<std::string_view>>
        // {
        //     auto send_msg = co_await cobalt::this_coro::initial;
        //     auto data_size = std::size_t{ 0 };
        //     while (true)
        //     {
        //         if (not send_msg.has_value())
        //         {
        //             close();
        //             co_return 0;
        //         }
        //         data_size = (not send_msg.value().empty())
        //                         ? get_socket_ptr()->send_to(asio::buffer(send_msg.value()), get_remote_endpoint())
        //                         : 0;
        //         send_msg = co_yield data_size;
        //     }
        //     co_return 0;
        // }

        auto send_continuous_message() -> asio::experimental::coro<OutputType(std::optional<InputType>)>
        {
            auto* socket = get_socket_ptr();
            auto total_size = std::size_t{};
            spdlog::debug("UDP: Starting to send data to the remote point {}", get_remote_endpoint());
            auto msg = co_yield OutputType{};
            while (true)
            {
                if (not msg.has_value())
                {
                    break;
                }
                const auto read_size =
                    (not msg.value().empty()) ? socket->send_to(asio::buffer(msg.value()), get_remote_endpoint()) : 0;
                total_size += read_size;
                msg = co_yield read_size;
            }
            close();
            spdlog::debug("UDP: Stopped sending the message. Sent bytes: {}", total_size);
            co_return;
        }
        // auto send_continuous_message() -> asio::experimental::coro<std::size_t(std::optional<std::string_view>)>
        // {
        //     auto send_msg = std::string_view{};
        //     auto data_size = std::size_t{ 0 };
        //     while (true)
        //     {
        //         data_size = (not send_msg.empty())
        //                         ? get_socket_ptr()->send_to(asio::buffer(send_msg), get_remote_endpoint())
        //                         : 0;
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

      private:
        io_executor_type io_executor;
    };
} // namespace srs::connection

namespace srs::writer
{
    class UDP
    {
      public:
        UDP(App& app,
            asio::ip::udp::endpoint endpoint,
            process::DataConvertOptions deser_mode = process::DataConvertOptions::none)
            : convert_mode_{ deser_mode }
            , connection_{ { .name = "UDP writer", .buffer_size = common::LARGE_READ_MSG_BUFFER_SIZE },
                           app.get_io_context().get_executor() }
            , app_{ app }
        {
            connection_.set_socket(std::make_unique<asio::ip::udp::socket>(
                app.get_io_context(), asio::ip::udp::endpoint{ asio::ip::udp::v4(), 0 }));
            connection_.set_remote_endpoint(std::move(endpoint));
            coro_ = connection_.send_continuous_message();
            common::coro_sync_start(coro_, std::optional<std::string_view>{}, asio::use_awaitable);
        }

        static constexpr auto IsStructType = false;
        auto is_deserialize_valid() { return convert_mode_ == raw or convert_mode_ == proto; }

        auto get_convert_mode() const -> process::DataConvertOptions { return convert_mode_; }
        auto write(auto last_fut) -> boost::unique_future<std::optional<std::size_t>>
        {
            return common::create_coro_future(coro_, last_fut);
        }

        // INFO: this will be called in coroutine
        void close() { connection_.close(); }

        // Getters:
        auto get_local_socket() -> const auto& { return connection_.get_socket(); }
        auto get_remote_endpoint() -> const auto& { return connection_.get_remote_endpoint(); }

      private:
        using enum process::DataConvertOptions;
        process::DataConvertOptions convert_mode_;
        connection::UDPWriterConnection connection_;
        std::reference_wrapper<App> app_;
        asio::experimental::coro<std::size_t(std::optional<std::string_view>)> coro_;
    };

} // namespace srs::writer
