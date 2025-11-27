#pragma once

#include "srs/connections/ConnectionTypeDef.hpp"
#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/utils/CommonAlias.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/uses_executor.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/thread/future.hpp>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <srs/connections/ConnectionBase.hpp>
#include <srs/writers/DataWriterOptions.hpp>
#include <string_view>
#include <vector>

namespace srs::connection
{
    class UDPWriterConnection : public Base
    {
      public:
        explicit UDPWriterConnection(const Config& config, io_context_type& io_executor)
            // : Base(info, "UDP writer")
            : Base(config)
            , io_context_{ &io_executor }
        {
        }

        auto get_executor() const { return io_context_->get_executor(); }
        using OutputType = std::size_t;
        using InputType = std::string_view;

        auto send_sync_message(InputType input_data) -> OutputType
        {
            auto* socket = get_socket_ptr();
            const auto read_size = (not input_data.empty())
                                       ? socket->send_to(asio::buffer(input_data), get_remote_endpoint(), 0, err_)
                                       : 0;
            return read_size;
        }

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

      private:
        boost::system::error_code err_;
        io_context_type* io_context_ = nullptr;
    };
} // namespace srs::connection

namespace srs::writer
{
    class UDP : public process::WriterTask<DataWriterOption::udp, std::string_view, std::size_t>
    {
      public:
        UDP(io_context_type& io_context,
            asio::ip::udp::endpoint remote_endpoint,
            std::size_t n_lines,
            process::DataConvertOptions deser_mode = process::DataConvertOptions::none);
        static constexpr auto IsStructType = false;

        ~UDP();
        UDP(const UDP&) = default;
        UDP(UDP&&) = delete;
        UDP& operator=(const UDP&) = default;
        UDP& operator=(UDP&&) = delete;

        void run_task(const auto& prev_data_converter, std::size_t line_number)
        {
            assert(line_number < get_n_lines());
            output_data_[line_number] =
                connections_[line_number]->send_sync_message(prev_data_converter.get_data_view(line_number));
        }

        [[nodiscard]] auto is_deserialize_valid() const
        {
            return get_required_conversion() == raw or get_required_conversion() == proto;
        }
        [[nodiscard]] auto get_data(std::size_t line_number) const -> OutputType { return output_data_[line_number]; }

      private:
        std::vector<OutputType> output_data_;
        std::vector<std::unique_ptr<connection::UDPWriterConnection>> connections_;
        using enum process::DataConvertOptions;
    };

} // namespace srs::writer
