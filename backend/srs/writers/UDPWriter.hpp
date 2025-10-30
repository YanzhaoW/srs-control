#pragma once

#include "srs/Application.hpp"
#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/utils/CommonFunctions.hpp"
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/thread/future.hpp>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
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
        explicit UDPWriterConnection(const Info& info)
            : Base(info, "UDP writer")
        {
        }
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
            , connection_{ connection::Info{ &app } }
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
