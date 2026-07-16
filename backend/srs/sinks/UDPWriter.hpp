#pragma once

#include "srs/converters/DataConvertOptions.hpp"
#include "srs/sinks/DataWriterOptions.hpp"
#include "srs/utils/CommonAlias.hpp"
#include "srs/utils/CommonConcepts.hpp"
#include "srs/utils/UDPFormatters.hpp" // IWYU pragma: keep
#include "srs/workflow/BaseTask.hpp"
#include <asio/buffer.hpp>
#include <asio/experimental/coro.hpp>
#include <asio/ip/udp.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/uses_executor.hpp>
#include <cassert>
#include <cstddef>
#include <memory>
#include <spdlog/spdlog.h>
#include <string_view>
#include <system_error>
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

        void close()
        {
            auto error = std::error_code{};
            socket_.shutdown(asio::ip::udp::socket::shutdown_send, error);
            socket_.close(error);
            if (error)
            {
                spdlog::warn("UDP: Failed to close the socket with endpoint: {} due to the error: {}",
                             socket_.local_endpoint(),
                             error.message());
            }
        }

      private:
        std::error_code err_;
        io_context_type* io_context_ = nullptr;
        asio::ip::udp::endpoint remote_endpoint_;
        asio::ip::udp::socket socket_;
    };
} // namespace srs::connection

namespace srs::sink
{
    class UDP : public process::SinkTask<DataWriterOption::udp, std::string_view, std::size_t>
    {
      public:
        UDP(io_context_type& io_context,
            asio::ip::udp::endpoint remote_endpoint,
            std::size_t n_lines,
            process::DataConvertOptions deser_mode = process::DataConvertOptions::none);
        static constexpr auto IsStructType = false;

        ~UDP() noexcept;
        UDP(const UDP&) = delete;
        UDP(UDP&&) = default;
        UDP& operator=(const UDP&) = delete;
        UDP& operator=(UDP&&) = default;

        auto run(const OutputTo<InputType> auto& prev_data_converter, std::size_t line_number = 0) -> RunResult
        {
            assert(line_number < get_n_lines());
            output_data_[line_number] = connections_[line_number]->send_sync_message(prev_data_converter(line_number));
            return output_data_[line_number];
        }

        [[nodiscard]] auto is_deserialize_valid() const
        {
            return get_required_conversion() == raw or get_required_conversion() == proto;
        }
        [[nodiscard]] auto operator()(std::size_t line_number = 0) const -> OutputType
        {
            return output_data_[line_number];
        }

      private:
        std::vector<OutputType> output_data_;
        std::vector<std::unique_ptr<connection::UDPWriterConnection>> connections_;
        using enum process::DataConvertOptions;
    };

} // namespace srs::sink
