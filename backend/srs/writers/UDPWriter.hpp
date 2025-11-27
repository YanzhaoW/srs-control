#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/converters/DataConverterBase.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonConcepts.hpp"
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
#include <utility>
#include <vector>

namespace srs::connection
{
    class UDPWriterConnection
    {
      public:
        explicit UDPWriterConnection(io_context_type& io_executor, asio::ip::udp::endpoint remote_endpoint)
            : io_context_{ &io_executor }
            , remote_endpoint_{ std::move(remote_endpoint) }
            , socket_{ io_executor, asio::ip::udp::endpoint{ asio::ip::udp::v4(), 0 } }
        {
        }
        using OutputType = std::size_t;
        using InputType = std::string_view;

        [[nodiscard]] auto get_executor() const { return io_context_->get_executor(); }

        auto send_sync_message(InputType input_data) -> OutputType
        {
            const auto read_size =
                (not input_data.empty()) ? socket_.send_to(asio::buffer(input_data), remote_endpoint_, 0, err_) : 0;
            return read_size;
        }

        void close() { socket_.close(); }

        auto send_continuous_message() -> asio::experimental::coro<OutputType(std::optional<InputType>)>
        {
            auto total_size = std::size_t{};
            spdlog::debug("UDP: Starting to send data to the remote point {}", remote_endpoint_);
            auto msg = co_yield OutputType{};
            while (true)
            {
                if (not msg.has_value())
                {
                    break;
                }
                const auto read_size =
                    (not msg.value().empty()) ? socket_.send_to(asio::buffer(msg.value()), remote_endpoint_) : 0;
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
        asio::ip::udp::endpoint remote_endpoint_;
        asio::ip::udp::socket socket_;
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

        auto operator()(const OutputTo<InputType> auto& prev_data_converter, std::size_t line_number) -> OutputType
        {
            assert(line_number < get_n_lines());
            output_data_[line_number] =
                connections_[line_number]->send_sync_message(prev_data_converter.get_data_view(line_number));
            return output_data_[line_number];
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
